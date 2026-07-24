#include "internals.h"

// Freelist has a lot of possible cool implementations
//! Read about trees (can attempt next?)

static free_block_t* free_list_head = NULL;
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

void freelist_insert(free_block_t* block){
    block->is_free = 1;
    block->prev = NULL;
    block->next = free_list_head;
    if (free_list_head != NULL){
        free_list_head->prev = block;
    }
    free_list_head = block;
    // write footer so boundary tags stay consistent
    block_footer_t* footer = (block_footer_t*)((char*)block +sizeof(block_header_t) +block->size);
    footer->size =block->size;
    footer->is_free =1;
}

void freelist_remove(free_block_t* block){
    if (block->prev != NULL){
        block->prev->next = block->next;
    } 
    else{
        free_list_head = block->next;
    }
    if (block->next != NULL){
        block->next->prev = block->prev;
    }
    block->next = NULL;
    block->prev = NULL;
}

// first fit
free_block_t* freelist_search(size_t size){
    free_block_t* current = free_list_head;
    while(current != NULL){
        if(current->size >= size){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

free_block_t* freelist_split(free_block_t* block, size_t size){
    size_t remaining = block->size - size - BLOCK_OVERHEAD;
    // too little to give a damn abt
    if(block->size <(size +BLOCK_OVERHEAD +ALIGNMENT)){
        freelist_remove(block);
        return block;
    }
    freelist_remove(block);

    // shrink this block to reqd size
    block_header_t* header =(block_header_t*)block;
    header->size = size;
    header->is_free = 0;
    block_footer_t* footer = (block_footer_t*)((char*)header + sizeof(block_header_t) + size);
    footer->size = size;
    footer->is_free = 0;

    free_block_t* remainder = (free_block_t*)((char*)footer + sizeof(block_footer_t));
    remainder->size = remaining;
    remainder->is_free = 1;
    remainder->next = NULL;
    remainder->prev = NULL;

    // footer for remainder
    block_footer_t* rem_footer =(block_footer_t*)((char*)remainder +sizeof(block_header_t) +remaining);
    rem_footer->size =remaining;
    rem_footer->is_free =1;

    freelist_insert(remainder);
    return (free_block_t*)header;
}

// merge with the block right after in memory
void coalesce_next(free_block_t* block){
    block_header_t* next_header =(block_header_t*)((char*)block +sizeof(block_header_t) +block->size +sizeof(block_footer_t));
    if((void*)next_header>=heap_end)
    return;
    if(!next_header->is_free)
        return;

    free_block_t* next_block =(free_block_t*)next_header;
    freelist_remove(next_block);
    block->size += sizeof(block_header_t) +next_block->size +sizeof(block_footer_t);
    block_footer_t* footer =(block_footer_t*)((char*)block +sizeof(block_header_t) +block->size);
    footer->size = block->size;
    footer->is_free =1;
}

void coalesce_prev(free_block_t* block){
    if((void*)block<=heap_start)
        return;
    block_footer_t* prev_footer =(block_footer_t*)((char*)block-sizeof(block_footer_t));
    if((void*)prev_footer<heap_start)
        return;
    if(!prev_footer->is_free)
        return;
    block_header_t* prev_header =(block_header_t*)((char*)block -sizeof(block_footer_t) -prev_footer->size -sizeof(block_header_t));
    if((void*)prev_header<heap_start)
        return;

    free_block_t* prev_block =(free_block_t*)prev_header;
    freelist_remove(prev_block);
    freelist_remove(block);
    prev_block->size +=sizeof(block_footer_t) +sizeof(block_header_t) +block->size;

    block_footer_t* footer =(block_footer_t*)((char*)prev_block +sizeof(block_header_t) +prev_block->size);
    footer->size =prev_block->size;
    footer->is_free =1;
    freelist_insert(prev_block);
}

free_block_t* coalesce(free_block_t* block){
    coalesce_next(block);
    if((void*)block>heap_start){
        block_footer_t* prev_footer =(block_footer_t*)((char*)block-sizeof(block_footer_t));
        if((void*)prev_footer>=heap_start && prev_footer->is_free){
            block_header_t* prev_header =(block_header_t*)((char*)block -sizeof(block_footer_t) -prev_footer->size -sizeof(block_header_t));
            if((void*)prev_header>=heap_start){
                free_block_t* prev_block =(free_block_t*)prev_header;
                freelist_remove(prev_block);
                freelist_remove(block);
                prev_block->size += BLOCK_OVERHEAD + block->size;
                block_footer_t* footer =(block_footer_t*)((char*)prev_block + sizeof(block_header_t) + prev_block->size);
                footer->size =prev_block->size;
                footer->is_free =1;
                freelist_insert(prev_block);
                return prev_block;
            }
        }
    }
    return block;
}