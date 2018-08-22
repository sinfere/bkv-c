#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    u_int8_t* buf;
    size_t size;
} buffer;

typedef struct {
    int is_string_key;
    buffer* key;
    buffer* value;
} kv;

kv *kv_new_from_number_key(u_int64_t key, u_int8_t* value, size_t value_size);
kv *kv_new_from_string_key(char* key, u_int8_t* value, size_t value_size);

buffer *kv_pack(kv* t);

typedef struct {
    int code;
    kv* kv;
    buffer* remaining_buffer;
} kv_unpack_result;

kv_unpack_result *kv_unpack(u_int8_t* value, size_t size);

void kv_free(kv* t);
void kv_free_buffer(buffer* b);

char *kv_get_string_key(kv* t);
u_int64_t kv_get_number_key(kv* t);

struct bkv {
    kv* kvs;
};