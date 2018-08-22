#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bkv.h"

void dump_buffer(buffer* b) {
    printf("%ld ", b->size);
    for (int i = 0; i < b->size; i++) {
        printf("%02X", *(b->buf + i));
    }
    printf("\n");    
}

void test_size() {
    char *string = "Hello, world";
    int array[3] = {1, 2, 3};
    int *p = array;
    printf("int size: %ld, int* size: %ld \n", sizeof(int), sizeof(int*));
    printf("string %ld \n", sizeof(*string));
    printf("array %ld \n", sizeof(array));
    printf("pointer %ld \n", sizeof(p));    
}

void test_kv_encode_decode() {
    u_int8_t value[2] = {2, 3};
    kv* t = kv_new_from_number_key(1, &value[0], 2);

    buffer* b = kv_pack(t);
    printf("pack: ");
    dump_buffer(b);

    kv_unpack_result* r = kv_unpack(b->buf, b->size);
    printf("code: %d \n", r->code);
    printf("is_string_key: %d \n", r->kv->is_string_key);
    printf("key: ");
    dump_buffer(r->kv->key);
    printf("value: ");
    dump_buffer(r->kv->value);    
    printf("\n");

    kv_free(t);
    kv_free_buffer(b);
    kv_free(r->kv);
    kv_free_buffer(r->remaining_buffer);
    free(r);
}

void test_ml_kv_encode_decode() {
    while (1) {
        test_kv_encode_decode();
    }    
}


int main() {
    test_kv_encode_decode();
}

