#include "common.h"

buffer* buffer_new(u_int8_t* buf, size_t size) { 
    u_int8_t *new_buf = malloc(size * sizeof(u_int8_t));
    memcpy(new_buf, buf, size);

    buffer *b = malloc(sizeof(buffer));
    b->buf = new_buf;
    b->size = size;
    b->capacity = size;
    return b;
}

buffer* buffer_alloc(size_t capacity) {
    u_int8_t *new_buf = malloc(capacity * sizeof(u_int8_t));
    memset(new_buf, 0, capacity * sizeof(u_int8_t));

    buffer *b = malloc(sizeof(buffer));
    b->buf = new_buf;
    b->size = 0;
    b->capacity = capacity;
    return b;
}

buffer* buffer_clone(buffer* b) { 
    return buffer_new(b->buf, b->size);
}

int buffer_grow(buffer* b, size_t capacity) {
    if (capacity <= b->capacity) {
        return 0;
    }

    // if (b->size == 0) {
    //     u_int8_t *new_buf = malloc(capacity * sizeof(u_int8_t));
    //     memset(new_buf, 0, capacity * sizeof(u_int8_t));
    //     b->buf = new_buf;
    //     b->capacity = capacity;
    //     return 0;
    // }

    u_int8_t* buf = realloc(b->buf, capacity * sizeof(u_int8_t));
    if (!buf) {
        return -1;
    }

    b->buf = buf;
    b->capacity = capacity;
    
    return 0;
}

int buffer_append(buffer* b, u_int8_t* buf, size_t size) {
    if (size + b->size > b->capacity) {
        int grow_ret = buffer_grow(b, size + b->size);
        if (grow_ret != 0) {
            LOGE("grow fail");
            return -1;
        }
    }

    // LOGI("append size: %ld", size);

    memcpy(b->buf + b->size, buf, size);
    b->size = b->size + size;
    b->capacity = b->size;

    return 0;
}

void buffer_free(buffer* b) {
    if (b->buf != NULL) {
        free(b->buf);
    }
    
    free(b);
}