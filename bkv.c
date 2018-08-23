#include "bkv.h"

void reverse(u_int8_t * bs, size_t size) {
    for (int i = 0, j = size - 1; i < j; i++, j--) {
        u_int8_t tmp = *(bs + i);
        *(bs + i) = *(bs + j);
        *(bs + j) = tmp;
    }
}

buffer* encode_number(u_int64_t number) {
    buffer *b = malloc(sizeof(buffer));

    int buf_size = 0;
    u_int64_t number_for_count = number;
    while (number_for_count > 0) {
        buf_size++;
        number_for_count >>= 8;
    }

    u_int8_t *buf = (u_int8_t *) malloc(buf_size * sizeof(u_int8_t));
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
    if (buf_size > 8) {
        buf_size = 8;
    }
    u_int64_t n = 0;
    for (int i = 0; i < buf_size; i++) {
        n <<= 8;
        n |= buf[i];
    }

    return n;
}

buffer* encode_length(u_int64_t length) {
    buffer *b = malloc(sizeof(buffer));

    int buf_size = 0;
    u_int64_t length_for_count = length;
    while (length_for_count > 0) {
        buf_size++;
        length_for_count >>= 7;
    }

    u_int8_t *buf = (u_int8_t *) malloc(buf_size * sizeof(u_int8_t));
    int i = 0;
    while (length > 0) {
        *(buf + i) = (u_int8_t)(length & 0x7F);
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

typedef struct {
    int code;
    u_int64_t length;
    uint64_t length_byte_size;
} decode_length_result;

decode_length_result* decode_length(u_int8_t * buf, size_t buf_size) {
    decode_length_result *result = malloc(sizeof(decode_length_result));

    int length_byte_size = 0;
    for (int i = 0; i < buf_size; i++) {
        length_byte_size++;
        if ((*(buf + i) & 0x80) == 0) {
            break;
        }
    }

    if (length_byte_size == 0 || length_byte_size > 4) {
        // error: invalid length buf
        result->code = 1;
        return result;
    }

    u_int64_t length = 0;

    for (int i = 0; i < length_byte_size; i++) {
        length <<= 7;
        length |= *(buf + i);
    }

    result->code = 0;
    result->length = length;
    result->length_byte_size = length_byte_size;

    return result;
}

buffer* new_buffer(u_int8_t* buf, size_t buf_size) { 
    u_int8_t *new_buf = malloc(buf_size * sizeof(u_int8_t));
    memcpy(new_buf, buf, buf_size);

    buffer *b = malloc(sizeof(buffer));
    b->buf = new_buf;
    b->size = buf_size;
    return b;
}

buffer* clone_buffer(buffer* b) { 
    return new_buffer(b->buf, b->size);
}

kv* kv_new_from_number_key(u_int64_t key, u_int8_t* value, size_t size) {
    kv *t = malloc(sizeof(kv));
    t->is_string_key = 0;
    t->key = encode_number(key);
    t->value = new_buffer(value, size);
    return t;
}

kv* kv_new_from_string_key(char * key, u_int8_t* value, size_t size) {
    kv *t = malloc(sizeof(kv));
    t->is_string_key = 1;
    t->key = new_buffer((u_int8_t *)key, strlen(key));
    t->value = new_buffer(value, size);
    return t;    
}

kv* kv_clone(kv* t) {
    kv *nt = malloc(sizeof(kv));
    nt->is_string_key = t->is_string_key;
    nt->key = clone_buffer(t->key);
    nt->value = clone_buffer(t->value);
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
    
    u_int8_t* buf = malloc(buf_size * sizeof(u_int8_t));

    // variable length part
    memcpy(buf, length_encoded_buffer->buf, length_encoded_buffer->size);

    // key length byte
    *(buf + length_size) = keyLenghtByte;

    // key
    memcpy(buf + length_size + 1, t->key->buf, t->key->size);

    // value
    memcpy(buf + length_size + 1 + t->key->size, t->value->buf, t->value->size);

    buffer *b = new_buffer(buf, buf_size);

    kv_free_buffer(length_encoded_buffer);
    free(buf);

    return b;
}

kv_unpack_result* kv_unpack(u_int8_t* value, size_t size) {
    kv_unpack_result *r = malloc(sizeof(kv_unpack_result));
    r->code = 0;
    r->kv = NULL;
    r->size = 0;

    if (size == 0) {
        r->code = KV_UNPACK_RESULT_CODE_EMPTY_BUF;
        return r;
    }

    decode_length_result * dlr = decode_length(value, size);
    if (dlr->code != 0) {
        // decode length fail
        r->code = KV_UNPACK_RESULT_CODE_DECODE_LENGTH_FAIL;
        return r;
    }

    u_int64_t payload_length = dlr->length;
    int remaining_size = size - payload_length - dlr->length_byte_size;
    if (remaining_size < 0 || (size - dlr->length_byte_size <= 0)) {
        // buf not enough
        r->code = KV_UNPACK_RESULT_CODE_BUF_NOT_ENOUGH;
        return r;
    }

    u_int8_t* payload = value + dlr->length_byte_size;

    int is_string_key = 0;

    u_int8_t key_size_byte = *payload;
    u_int8_t key_size = key_size_byte & 0x7F;
    if ((key_size_byte & 0x80) != 0) {
        is_string_key = 1;
    }

    int value_size = payload_length - 1 - key_size;
    if (value_size < 0) {
        // wrong key size
        r->code = KV_UNPACK_RESULT_CODE_WRONG_KEY_SIZE;
        return r;
    }

    u_int8_t* key_buf = malloc(key_size * sizeof(u_int8_t));
    memcpy(key_buf, payload + 1, key_size);

    u_int8_t* value_buf = malloc(value_size * sizeof(u_int8_t));
    memcpy(value_buf, payload + 1 + key_size, value_size);

    kv* t = malloc(sizeof(kv));
    t->is_string_key = is_string_key;
    t->key = new_buffer(key_buf, key_size);
    t->value = new_buffer(value_buf, value_size);

    r->kv = t;
    r->size = payload_length + dlr->length_byte_size;

    free(key_buf);
    free(value_buf);
    free(dlr);

    return r;
}

void kv_free(kv* t) {
    kv_free_buffer(t->key);
    kv_free_buffer(t->value);
    free(t);  
}

void kv_free_buffer(buffer* b) {
    if (b->buf != NULL) {
        free(b->buf);
    }
    
    free(b);
}

void kv_free_unpack_result(kv_unpack_result *r) {
    if (r->kv != NULL) {
        kv_free(r->kv);
    }
    
    free(r);    
}

char* kv_get_string_key(kv* t) {
    char* s = malloc((t->key->size + 1) * sizeof(char));
    memset(s, 0, t->key->size + 1);
    memcpy(s, t->key->buf, t->key->size);
    return s;
}

u_int64_t kv_get_number_key(kv* t) {
    return decode_number(t->key->buf, t->key->size);
}







bkv* bkv_new() {
    bkv *b = malloc(sizeof(bkv));
    b->kvs = NULL;
    b->size = 0;
    return b;       
}

void bkv_add(bkv* b, kv* t) {
    t = kv_clone(t);
    size_t kv_size = sizeof(kv*);
    kv** p = malloc(kv_size * (b->size + 1));
    // for (int i = 0; i < b->size; i++) {
    //     p[i] = b->kvs[i];
    // }
    if (b->size != 0) {
        memcpy(p, b->kvs, kv_size * b->size);
    }
    free(b->kvs);
    // p[b->size] = t;
    memcpy(p + b->size, &t, kv_size);
    b->kvs = p;
    b->size += 1; 
} 

void bkv_add_by_number_key(bkv* b, u_int64_t key, u_int8_t* value, size_t value_size) {
    kv* t = kv_new_from_number_key(key, value, value_size);
    bkv_add(b, t);
    kv_free(t);
}

void bkv_add_by_string_key(bkv* b, char* key, u_int8_t* value, size_t value_size) {
    kv* t = kv_new_from_string_key(key, value, value_size);
    bkv_add(b, t);
    kv_free(t);
}

buffer* bkv_pack(bkv* b) {    
    if (b->size == 0) {
        return new_buffer(NULL, 0);
    }

    // first buf as base buffer
    buffer* tb = kv_pack(b->kvs[0]);
    for (int i = 1; i < b->size; i++) {
        buffer* pb = kv_pack(b->kvs[i]);
        size_t new_size = tb->size + pb->size;
        u_int8_t *new_buf = malloc(new_size * sizeof(u_int8_t));
        memcpy(new_buf, tb->buf, tb->size);
        memcpy(new_buf + tb->size, pb->buf, pb->size);
        
        free(tb->buf);
        tb->buf = new_buf;
        tb->size = new_size;
        kv_free_buffer(pb);
    }

    return tb;
}

bkv_unpack_result* bkv_unpack(u_int8_t* buf, size_t buf_size) {
    bkv_unpack_result *br = malloc(sizeof(bkv_unpack_result));
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
    for (int i = 0; i < b->size; i++) {
        kv* pt = *(b->kvs + i);
        kv_free(pt);
    }
    if (b->kvs != NULL) {
        free(b->kvs);
    }
    
    free(b);
}

void bkv_free_unpack_result(bkv_unpack_result *r) {
    if (r->bkv != NULL) {
        bkv_free(r->bkv);
    }
    free(r);        
}














void dump_buffer(char* name, buffer* b) {
    printf("%-30s ", name);
    for (int i = 0; i < b->size; i++) {
        printf("%02X", *(b->buf + i));
    }
    printf(" len: %ld \n", b->size);    
}

void dump_kv(kv* t) {
    if (t == NULL) {
        return;
    }
    printf("%-30s %d \n", "kv.is_string_key:", t->is_string_key);
    dump_buffer("kv.key", t->key);
    if (t->is_string_key) {
        char* string_key = kv_get_string_key(t);
        printf("%-30s %s \n", "kv.string_key:", string_key);
        free(string_key);
    } else {
        u_int64_t number_key = kv_get_number_key(t);
        printf("%-30s %lld \n", "kv.number_key:", number_key);
    }
    dump_buffer("kv.value", t->value);   
    printf("\n"); 
}

void dump_bkv(bkv* b) {
    for (int i = 0; i < b->size; i++) {
        kv* t = *(b->kvs + i);
        dump_kv(t);
    }  
}
