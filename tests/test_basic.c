#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "lethe.h"

static void test_returns_non_null(void){
    void* p = malloc(64);
    assert(p != NULL);
    free(p);
    printf("test_returns_non_null passed\n");
}

static void test_writable(void){
    char* p = malloc(128);
    assert(p != NULL);
    memset(p, 0xAB, 128);
    for (int i = 0;i<128;i++){
        assert((unsigned char)p[i] == 0xAB);
    }
    free(p);
    printf("test_writable passed\n");
}

static void test_free_does_not_crash(void){
    void* p = malloc(32);
    free(p);
    free(NULL);
    printf("test_free_does_not_crash passed\n");
}

static void test_alloc_free_cycles(void){
    for (int i = 0;i<1000;i++){
        void* p = malloc(48);
        assert(p != NULL);
        free(p);
    }
    printf("test_alloc_free_cycles passed\n");
}

static void test_multiple_live_allocations(void){
    void* blocks[16];
    for (int i = 0;i<16;i++){
        blocks[i] = malloc(64);
        assert(blocks[i] != NULL);
        memset(blocks[i], i, 64);
    }

    for (int i = 0;i<16;i++){
        char* p =(char*)blocks[i];
        for (int j = 0;j<64;j++){
            assert((unsigned char)p[j] == i);
        }
    }
    for (int i = 0;i<16;i++){
        free(blocks[i]);
    }
    printf("test_multiple_live_allocations passed\n");
}

static void test_large_allocation_mmap_path(void){
    size_t big = 2*1024*1024;
    char* p = malloc(big);
    assert(p != NULL);

    p[0] ='a';
    p[big-1] ='z';
    assert(p[0]=='a');
    assert(p[big-1] =='z');
    free(p);
    printf("test_large_allocation_mmap_path passed\n");
}

static void test_zero_size_returns_null(void){
    void* p = malloc(0);
    assert(p == NULL);
    printf("test_zero_size_returns_null passed\n");
}

//! realloc tests

static void test_realloc_null_is_malloc(void){
    void* p = realloc(NULL, 64);
    assert(p != NULL);
    free(p);
    printf("test_realloc_null_is_malloc passed\n");
}

static void test_realloc_zero_is_free(void){
    void* p = malloc(64);
    assert(p != NULL);
    void* q = realloc(p, 0);
    assert(q == NULL);
    printf("test_realloc_zero_is_free passed\n");
}

static void test_realloc_grow_preserves_data(void){
    char* p = malloc(32);
    assert(p != NULL);
    memset(p, 0xAA, 32);

    p = realloc(p, 128);
    assert(p != NULL);
    // first 32 bytes must still be intact
    for (int i = 0; i < 32; i++){
        assert((unsigned char)p[i] == 0xAA);
    }
    free(p);
    printf("test_realloc_grow_preserves_data passed\n");
}

static void test_realloc_shrink(void){
    char* p = malloc(256);
    assert(p != NULL);
    memset(p, 0xBB, 256);

    p = realloc(p, 32);
    assert(p != NULL);
    for (int i = 0; i < 32; i++){
        assert((unsigned char)p[i] == 0xBB);
    }
    free(p);
    printf("test_realloc_shrink passed\n");
}

//! calloc tests

static void test_calloc_zeroed(void){
    char* p = calloc(16, 32);
    assert(p != NULL);
    for (int i = 0; i < 16 * 32; i++){
        assert(p[i] == 0);
    }
    free(p);
    printf("test_calloc_zeroed passed\n");
}

static void test_calloc_overflow(void){
    // SIZE_MAX / 2 + 1 * 2 would overflow
    void* p = calloc(SIZE_MAX / 2 + 1, 2);
    assert(p == NULL);
    printf("test_calloc_overflow passed\n");
}

int main(void){
    test_returns_non_null();
    test_writable();
    test_free_does_not_crash();
    test_alloc_free_cycles();
    test_multiple_live_allocations();
    test_large_allocation_mmap_path();
    test_zero_size_returns_null();

    test_realloc_null_is_malloc();
    test_realloc_zero_is_free();
    test_realloc_grow_preserves_data();
    test_realloc_shrink();

    test_calloc_zeroed();
    test_calloc_overflow();

    printf("all tests passed\n");
    return 0;
}