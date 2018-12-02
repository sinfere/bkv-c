#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bkv.h"

void test_size() {
    char *string = "Hello, world";
    int array[3] = {1, 2, 3};
    int *p = array;
    printf("int size: %ld, int* size: %ld \n", sizeof(int), sizeof(int*));
    printf("string %ld \n", sizeof(*string));
    printf("array %ld \n", sizeof(array));
    printf("pointer %ld \n", sizeof(p));  

    printf("buffer type size: %ld %ld \n", sizeof(buffer), sizeof(u_int8_t*));  
}

void test_kv_encode_decode_number() {
    printf("[kv-encode-decode-number-key]\n");
    u_int8_t value[2] = {2, 3};
    kv* t = kv_new_from_number_key(2086, buffer_new(value, 2));

    buffer* b = kv_pack(t);
    dump_buffer("pack result:", b);

    kv_unpack_result* r = kv_unpack(b->buf, b->size);
    printf("%-30s %d \n", "unpack result code:", r->code);
    dump_kv(r->kv);
    printf("\n");

    kv_free(t);
    kv_free_buffer(b);
    kv_free_unpack_result(r);
}

void test_kv_encode_decode_string() {
    printf("[kv-encode-decode-string-key]\n");
    u_int8_t value[2] = {2, 3};
    kv* t = kv_new_from_string_key("protocol", buffer_new(value, 2));

    buffer* b = kv_pack(t);
    dump_buffer("pack result:", b);

    kv_unpack_result* r = kv_unpack(b->buf, b->size);
    printf("%-30s %d \n", "unpack result code:", r->code);
    dump_kv(r->kv);
    printf("\n");

    kv_free(t);
    kv_free_buffer(b);
    kv_free_unpack_result(r);
}

void test_bkv_encode_decode() {
    // printf("[bkv-encode-decode-number-key]\n");
    bkv* tb = bkv_new();

    u_int8_t value[2] = {2, 3};
    bkv_add_by_number_key(tb, 1, buffer_new(value, 2));
    bkv_add_by_string_key(tb, "version", buffer_new_from_number(515));

    bkv_add_by_string_key(tb, "test", buffer_new_from_string("hello"));

    buffer* b = bkv_pack(tb);
    bkv_free(tb);

    dump_buffer("pack result:", b);
    printf("\n");

    bs_debug();

    bkv_unpack_result* r = bkv_unpack(b->buf, b->size);
    if (r->code != 0) {
        LOGE("unpack fail");
    }
    kv_free_buffer(b);
    
    printf("%-30s %d \n", "unpack result code:", r->code);
    printf("%-30s %ld \n\n", "unpack kv size:", r->bkv->size);

    dump_bkv(r->bkv);
    printf("\n");

    bkv_free_unpack_result(r);

    bs_debug();
}

void test_bkv_encode_decode_simple() {
    // printf("[bkv-encode-decode-number-key]\n");
    bkv* tb = bkv_new();

    bkv_add_by_number_key(tb, 1, buffer_new_from_number(515));
    bkv_add_by_string_key(tb, "version", buffer_new_from_number(515));
    bkv_add_by_string_key(tb, "test", buffer_new_from_string("hello"));

    buffer* b = bkv_pack(tb);

    dump_buffer("pack result:", b);
    printf("\n");

    bkv_unpack_result* r = bkv_unpack(b->buf, b->size);
    if (r->code != 0) {
        LOGE("unpack fail");
    }
    
    printf("%-30s %d \n", "unpack result code:", r->code);
    printf("%-30s %ld \n\n", "unpack kv size:", r->bkv->size);

    dump_bkv(r->bkv);
    printf("\n");

    bkv_free(tb);
    kv_free_buffer(b);
    bkv_free_unpack_result(r);
}

void test_ml_kv_encode_decode() {
    while (1) {
        test_kv_encode_decode_number();
        test_kv_encode_decode_string();
    }    
}

void test_ml_bkv_encode_decode() {
    while (1) {
        test_bkv_encode_decode();
    }    
}

// ~ 6 seconds
void test_bench_bkv_encode_decode() {
    int i = 1000000;
    while (i-- >= 0) {
        test_bkv_encode_decode();
    }   
}

void test_encode_decode_length() {
    buffer* b = encode_length(88888888);
    dump_buffer("length", b);
    decode_length_result *dr = decode_length(b->buf, b->size);
    LOGI("%d", dr->code);
    LOGI("%d", (int)dr->length);
}


int main() {
    b_init();

    printf("begin: %d\n\n",(int)time(NULL));

    // test_ml_kv_encode_decode();
    // test_kv_encode_decode_number();
    // test_kv_encode_decode_string();

    test_bench_bkv_encode_decode();
    // test_ml_bkv_encode_decode();
    // test_bkv_encode_decode();

    // test_encode_decode_length();

    // test_bkv_encode_decode_simple();

    printf("end: %d\n",(int)time(NULL));

    return 0;
}

