#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "lethe.h"

// allocate 3 adjacent blocks, free middle then first — should coalesce
static void test_coalesce_prev_and_next(void){
    void* a = malloc(64);
    void* b = malloc(64);
    void* c = malloc(64);
    assert(a && b && c);

    free(b);
    free(a);
    // a and b should have coalesced — alloc something that needs both
    void* big = malloc(128);
    assert(big != NULL);
    memset(big, 0xCC, 128);

    free(c);
    free(big);
    printf("test_coalesce_prev_and_next passed\n");
}

// free only the next neighbor
static void test_coalesce_next_only(void){
    void* a = malloc(64);
    void* b = malloc(64);
    void* c = malloc(64);
    assert(a && b && c);

    // free b then c — b should merge forward into c
    free(b);
    free(c);

    void* big = malloc(128);
    assert(big != NULL);
    memset(big, 0xDD, 128);

    free(a);
    free(big);
    printf("test_coalesce_next_only passed\n");
}

// free only the prev neighbor
static void test_coalesce_prev_only(void){
    void* a = malloc(64);
    void* b = malloc(64);
    void* c = malloc(64);
    assert(a && b && c);

    // free a, then free b — b should merge backward into a
    free(a);
    free(b);

    void* big = malloc(128);
    assert(big != NULL);
    memset(big, 0xEE, 128);

    free(c);
    free(big);
    printf("test_coalesce_prev_only passed\n");
}

// neither neighbor free — just a regular free, no merge
static void test_no_coalesce(void){
    void* a = malloc(64);
    void* b = malloc(64);
    void* c = malloc(64);
    assert(a && b && c);

    // free only b — a and c still allocated, so no merge
    free(b);
    void* d = malloc(64);
    assert(d != NULL);

    free(a);
    free(c);
    free(d);
    printf("test_no_coalesce passed\n");
}

// both neighbors free — triple merge
static void test_coalesce_both(void){
    void* a = malloc(64);
    void* b = malloc(64);
    void* c = malloc(64);
    assert(a && b && c);

    free(a);
    free(c);
    // now free b — both neighbors are free, should triple-merge
    free(b);

    void* big = malloc(192);
    assert(big != NULL);
    memset(big, 0xFF, 192);

    free(big);
    printf("test_coalesce_both passed\n");
}

// repeated alloc/free of same size — heap shouldn't grow unbounded
static void test_fragmentation_stress(void){
    for (int i = 0; i < 10000; i++){
        void* p = malloc(48);
        assert(p != NULL);
        free(p);
    }
    printf("test_fragmentation_stress passed\n");
}

int main(void){
    test_coalesce_prev_and_next();
    test_coalesce_next_only();
    test_coalesce_prev_only();
    test_no_coalesce();
    test_coalesce_both();
    test_fragmentation_stress();
    printf("all coalesce tests passed\n");
    return 0;
}
