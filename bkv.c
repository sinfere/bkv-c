#include "bkv.h"
#include <inttypes.h>

void reverse(u_int8_t * bs, size_t size) {
    int i, j;
    for (i = 0, j = size - 1; i < j; i++, j--) {
        u_int8_t tmp = *(bs + i);
        *(bs + i) = *(bs + j);
        *(bs + j) = tmp;
    }
}

buffer* encode_number(u_int64_t number) {
    buffer *b = b_malloc(sizeof(buffer));

    int buf_size = 0;
    u_int64_t number_for_count = number;
    while (number_for_count > 0) {
        buf_size++;
        number_for_count >>= 8;
    }

    u_int8_t *buf = (u_int8_t *) b_malloc(buf_size * sizeof(u_int8_t));
    int i = 0;
    while (number > 0) {
        *(buf + i) = (u_int8_t)(number & 0xFF);
        number >>= 8;
        i++;
    }

    reverse(buf, buf_size);

    b->buf = buf;
    b->size = buf_size;

    return b;
}

u_int64_t decode_number(u_int8_t * buf, size_t buf_size) {
    int i;

    if (buf_size > 8) {
        buf_size = 8;
    }
    u_int64_t n = 0;
    for (i = 0; i < buf_size; i++) {
        n <<= 8;
        n |= buf[i];
    }

    return n;
}

buffer* encode_length(u_int64_t length) {
    buffer *b = b_malloc(sizeof(buffer));

    int buf_size = 0;
    u_int64_t length_for_count = length;
    while (length_for_count > 0) {
        buf_size++;
        length_for_count >>= 7;
    }

    u_int8_t *buf = (u_int8_t *) b_malloc(buf_size * sizeof(u_int8_t));
    int i = 0;
    while (length > 0) {
        *(buf + i) = (u_int8_t)((length & 0x7F) | 0x80);
        length >>= 7;
        i++;
    }

    reverse(buf, buf_size);

    u_int8_t last_byte = *(buf + buf_size - 1);
    last_byte &= 0x7F;
    *(buf + buf_size - 1) = last_byte;

    b->buf = buf;
    b->size = buf_size;

    return b;
}

decode_length_result* decode_length(u_int8_t * buf, size_t buf_size) {
    decode_length_result *result = b_malloc(sizeof(decode_length_result));

    int length_byte_size = 0;
    int i;
    for (i = 0; i < buf_size; i++) {
        length_byte_size++;
        if ((*(buf + i) & 0x80) == 0) {
            break;
        }
    }

    if (length_byte_size == 0 || length_byte_size > 4) {
        // error: invalid length buf
        LOGE("invalid length buf");
        result->code = 1;
        return result;
    }

    u_int64_t length = 0;

    for (i = 0; i < length_byte_size; i++) {
        length <<= 7;
        length |= *(buf + i) & 0x7F;
    }

    result->code = 0;
    result->length = length;
    result->length_byte_size = length_byte_size;

    return result;
}

kv* kv_new_from_number_key(u_int64_t key, u_int8_t* value, size_t size) {
    kv *t = b_malloc(sizeof(kv));
    t->is_string_key = 0;
    t->key = encode_number(key);
    t->value = buffer_new(value, size);
    return t;
}

kv* kv_new_from_string_key(char * key, u_int8_t* value, size_t size) {
    kv *t = b_malloc(sizeof(kv));
    t->is_string_key = 1;
    t->key = buffer_new((u_int8_t *)key, strlen(key));
    t->value = buffer_new(value, size);
    return t;    
}

kv* kv_clone(kv* t) {
    kv *nt = b_malloc(sizeof(kv));
    nt->is_string_key = t->is_string_key;
    nt->key = buffer_clone(t->key);
    nt->value = buffer_clone(t->value);
    return nt;    
}

buffer* kv_pack(kv * t) {
    u_int64_t payload_length = t->key->size + t->value->size + 1;
    buffer* length_encoded_buffer = encode_length(payload_length);
    size_t length_size = length_encoded_buffer->size;
    size_t buf_size = length_size + payload_length;

    u_int8_t keyLenghtByte = t->key->size & 0x7F;
    if (t->is_string_key) {
        keyLenghtByte |= 0x80;
    }
    
    u_int8_t* buf = b_malloc(buf_size * sizeof(u_int8_t));

    // variable length part
    memcpy(buf, length_encoded_buffer->buf, length_encoded_buffer->size);

    // key length byte
    *(buf + length_size) = keyLenghtByte;

    // key
    memcpy(buf + length_size + 1, t->key->buf, t->key->size);

    // value
    memcpy(buf + length_size + 1 + t->key->size, t->value->buf, t->value->size);

    buffer *b = buffer_new(buf, buf_size);

    kv_free_buffer(length_encoded_buffer);
    b_free(buf);

    return b;
}

kv_unpack_result* kv_unpack(u_int8_t* value, size_t size) {
    // dump_buffer("debug unpack", buffer_new(value, size));

    kv_unpack_result *r = b_malloc(sizeof(kv_unpack_result));
    r->code = 0;
    r->kv = NULL;
    r->size = 0;

    if (size == 0) {
        r->code = KV_UNPACK_RESULT_CODE_EMPTY_BUF;
        return r;
    }

    decode_length_result * dlr = decode_length(value, size);
    if (dlr->code != 0 || dlr->length == 0) {
        // decode length fail
        r->code = KV_UNPACK_RESULT_CODE_DECODE_LENGTH_FAIL;
        return r;
    }

    u_int64_t payload_length = dlr->length;
    int remaining_size = size - payload_length - dlr->length_byte_size;
    if (remaining_size < 0 || (size - dlr->length_byte_size <= 0)) {
        // buf not enough
        // LOGE("buf not enought: %d", (int)payload_length);
        r->code = KV_UNPACK_RESULT_CODE_BUF_NOT_ENOUGH;
        return r;
    }

    u_int8_t* payload = value + dlr->length_byte_size;

    int is_string_key = 0;

    u_int8_t key_length_byte = *payload;
    u_int8_t key_length = key_length_byte & 0x7F;
    if ((key_length_byte & 0x80) != 0) {
        is_string_key = 1;
    }

    int value_length = payload_length - 1 - key_length;
    if (value_length < 0) {
        // wrong key size
        r->code = KV_UNPACK_RESULT_CODE_WRONG_KEY_LENGTH;
        return r;
    }

    u_int8_t* key_buf = b_malloc(key_length * sizeof(u_int8_t));
    memcpy(key_buf, payload + 1, key_length);

    u_int8_t* value_buf = b_malloc(value_length * sizeof(u_int8_t));
    memcpy(value_buf, payload + 1 + key_length, value_length);

    kv* t = b_malloc(sizeof(kv));
    t->is_string_key = is_string_key;
    t->key = buffer_new(key_buf, key_length);
    t->value = buffer_new(value_buf, value_length);

    r->kv = t;
    r->size = payload_length + dlr->length_byte_size;

    b_free(key_buf);
    b_free(value_buf);
    b_free(dlr);

    return r;
}

void kv_free(kv* t) {
    kv_free_buffer(t->key);
    kv_free_buffer(t->value);
    b_free(t);  
}

void kv_free_buffer(buffer* b) {
    if (b->buf != NULL) {
        b_free(b->buf);
    }
    
    b_free(b);
}

void kv_free_unpack_result(kv_unpack_result *r) {
    if (r->kv != NULL) {
        kv_free(r->kv);
    }
    
    b_free(r);    
}

char* kv_get_string_key(kv* t) {
    char* s = b_malloc((t->key->size + 1) * sizeof(char));
    memset(s, 0, t->key->size + 1);
    memcpy(s, t->key->buf, t->key->size);
    return s;
}

u_int64_t kv_get_number_key(kv* t) {
    return decode_number(t->key->buf, t->key->size);
}







bkv* bkv_new() {
    bkv *b = b_malloc(sizeof(bkv));
    b->kvs = NULL;
    b->size = 0;
    return b;       
}

void bkv_add(bkv* b, kv* t) {
    t = kv_clone(t);
    size_t kv_size = sizeof(kv*);
    kv** p = b_malloc(kv_size * (b->size + 1));
    // for (int i = 0; i < b->size; i++) {
    //     p[i] = b->kvs[i];
    // }
    if (b->size != 0) {
        memcpy(p, b->kvs, kv_size * b->size);
    }
    b_free(b->kvs);
    // p[b->size] = t;
    memcpy(p + b->size, &t, kv_size);
    b->kvs = p;
    b->size += 1; 
} 

void bkv_add_by_number_key(bkv* b, u_int64_t key, u_int8_t* value, size_t value_length) {
    kv* t = kv_new_from_number_key(key, value, value_length);
    bkv_add(b, t);
    kv_free(t);
}

void bkv_add_by_string_key(bkv* b, char* key, u_int8_t* value, size_t value_length) {
    kv* t = kv_new_from_string_key(key, value, value_length);
    bkv_add(b, t);
    kv_free(t);
}

buffer* bkv_pack(bkv* b) {    
    if (b->size == 0) {
        return buffer_new(NULL, 0);
    }

    int i;

    // first buf as base buffer
    buffer* tb = kv_pack(b->kvs[0]);    
    for (i = 1; i < b->size; i++) {
        buffer* pb = kv_pack(b->kvs[i]);
        size_t new_size = tb->size + pb->size;
        u_int8_t *new_buf = b_malloc(new_size * sizeof(u_int8_t));
        memcpy(new_buf, tb->buf, tb->size);
        memcpy(new_buf + tb->size, pb->buf, pb->size);
        
        b_free(tb->buf);
        tb->buf = new_buf;
        tb->size = new_size;
        kv_free_buffer(pb);
    }

    return tb;
}

bkv_unpack_result* bkv_unpack(u_int8_t* buf, size_t buf_size) {
    bkv_unpack_result *br = b_malloc(sizeof(bkv_unpack_result));
    memset(br, 0, sizeof(bkv_unpack_result));
    bkv* b = bkv_new();
    br->bkv = b;

    int offset = 0;
    while (1) {
        kv_unpack_result* r = kv_unpack(buf + offset, buf_size - offset);

        offset += r->size;
        if (r->code == 0) {
            if (r->kv != NULL) {
                bkv_add(b, r->kv);
            }
            kv_free_unpack_result(r);
        } else {
            if (r->code == KV_UNPACK_RESULT_CODE_EMPTY_BUF) {
                br->code = 0;
                kv_free_unpack_result(r);
                return br; 
            }
            // unpack fail
            br->code = r->code;
            br->size = offset;
            kv_free_unpack_result(r);
            return br;
        }
    }
}

void bkv_free(bkv* b) {
    int i;

    for (int i = 0; i < b->size; i++) {
        kv* pt = *(b->kvs + i);
        kv_free(pt);
    }
    if (b->kvs != NULL) {
        b_free(b->kvs);
    }
    
    b_free(b);
}

void bkv_free_unpack_result(bkv_unpack_result *r) {
    if (r->bkv != NULL) {
        bkv_free(r->bkv);
    }
    b_free(r);        
}

kv* bkv_get_kv_from_string_key(bkv* b, char* key) {
    int i;

    for (i = 0; i < b->size; i++) {
        kv* t = *(b->kvs + i);
        if (t->is_string_key != 0) {
            char* t_key = kv_get_string_key(t);
            if (strcmp(t_key, key) == 0) {
                b_free(t_key);
                return t;
            }
        }
    }  

    return NULL;
}

kv* bkv_get_kv_from_number_key(bkv* b, u_int64_t key) {
    int i;

    for (i = 0; i < b->size; i++) {
        kv* t = *(b->kvs + i);
        if (t->is_string_key == 0) {
            u_int64_t t_key = kv_get_number_key(t);
            if (t_key == key) {
                return t;
            }
        }
    }  

    return NULL;
}

u_int64_t bkv_get_number_value_from_string_key(bkv* b, char* key) {
    kv* k = bkv_get_kv_from_string_key(b, key);
    if (k == NULL) {
        return 0;
    }
    return decode_number(k->value->buf, k->value->size);
}
char* bkv_get_string_value_from_string_key(bkv* b, char* key) {
    kv* k = bkv_get_kv_from_string_key(b, key);
    if (k == NULL) {
        return NULL;
    }
    
    char* s = b_malloc((k->value->size + 1) * sizeof(char));
    memset(s, 0, k->value->size + 1);
    memcpy(s, k->value->buf, k->value->size);
    return s;  
}












void dump_buffer(char* name, buffer* b) {
    int i;

    printf("%-5s[%ld]: ", name, b->size);
    for (i = 0; i < b->size; i++) {
        printf("%02X", *(b->buf + i));
    }

    // check printable string
    if (b->size != 0) {
        u_int8_t first_byte = *(b->buf);
        if (0x20 <= first_byte && first_byte <= 0x7E) {
            printf(" (s: ");
            for (int j = 0; j < b->size; j++) {
                printf("%c", *(b->buf + j));
            }
            printf(")");
        }
    }

    printf("\n");

}

void dump_kv(kv* t) {
    if (t == NULL) {
        return;
    }
    // printf("%-30s %d \n", "kv.is_string_key:", t->is_string_key);
    // dump_buffer("kv.key", t->key);
    if (t->is_string_key) {
        char* string_key = kv_get_string_key(t);
        printf("key[s]:%16s -> ", string_key);
        b_free(string_key);
    } else {
        u_int64_t number_key = kv_get_number_key(t);
        printf("key[n]:%16"PRIu64" -> ", number_key);
    }
    dump_buffer("value", t->value);   
}

void dump_bkv(bkv* b) {
    int i;
    
    for (i = 0; i < b->size; i++) {
        kv* t = *(b->kvs + i);
        dump_kv(t);
    }  
}
