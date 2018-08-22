#include "bkv.h"

void reverse(u_int8_t * bs, size_t size) {
    for (int i = 0, j = size - 1; i < j; i++, j--) {
        u_int8_t tmp = *(bs + i);
        *(bs + i) = *(bs + j);
        *(bs + j) = tmp;
    }
}

buffer *encode_number(u_int64_t number) {
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

buffer *encode_length(u_int64_t length) {
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

decode_length_result *decode_length(u_int8_t * buf, size_t buf_size) {
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

buffer *new_buffer(u_int8_t* buf, size_t buf_size) { 
    u_int8_t *new_buf = malloc(buf_size * sizeof(u_int8_t));
    memcpy(new_buf, buf, buf_size);

    buffer *b = malloc(sizeof(buffer));
    b->buf = new_buf;
    b->size = buf_size;
    return b;
}

kv *kv_new_from_number_key(u_int64_t key, u_int8_t* value, size_t size) {
    kv *t = malloc(sizeof(kv));
    t->is_string_key = 0;
    t->key = encode_number(key);
    t->value = new_buffer(value, size);
    return t;
}

kv *kv_new_from_string_key(char * key, u_int8_t* value, size_t size) {
    kv *t = malloc(sizeof(kv));
    t->is_string_key = 1;
    t->key = new_buffer((u_int8_t *)key, strlen(key));
    t->value = new_buffer(value, size);
    return t;    
}

buffer *kv_pack(kv * t) {
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

kv_unpack_result *kv_unpack(u_int8_t* value, size_t size) {
    kv_unpack_result *r = malloc(sizeof(kv_unpack_result));

    decode_length_result * dlr = decode_length(value, size);
    if (dlr->code != 0) {
        // decode length fail
        r->code = 1;
        return r;
    }

    u_int64_t payload_length = dlr->length;
    int remaining_size = size - payload_length - dlr->length_byte_size;
    if (remaining_size < 0 || (size - dlr->length_byte_size <= 0)) {
        // buf not enough
        r->code = 2;
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
        r->code = 3;
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
    r->remaining_buffer = new_buffer(payload + 1 + key_size + value_size, remaining_size);

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
    free(b->buf);
    free(b);
}