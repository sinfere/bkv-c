#include "common.h"

void b_init() {
#ifdef USE_TLSF
    init_memory_pool(TLSF_POOL_SIZE, b_mem_pool);
#endif
}

void* b_malloc(size_t size) {
#ifdef USE_TLSF
    return tlsf_malloc(size);
#else
    return malloc(size);
#endif
}

void* b_realloc(void* p, size_t size) {
#ifdef USE_TLSF
    return tlsf_realloc(p, size);
#else
    return realloc(p, size);
#endif
}

void b_free(void* p) {
#ifdef USE_TLSF
    tlsf_free(p);
#else
    free(p);
#endif
}

buffer* buffer_new(u_int8_t* buf, size_t size) { 
    u_int8_t* new_buf = b_malloc(size * sizeof(u_int8_t));
    memcpy(new_buf, buf, size);

    buffer* b = b_malloc(sizeof(buffer));
    b->buf = new_buf;
    b->size = size;
    b->capacity = size;
    return b;
}

buffer* buffer_new_from_number(u_int64_t n) { 
    return encode_number(n);
}

buffer* buffer_new_from_string(char* s) { 
    return buffer_new((u_int8_t*)s, strlen(s));
}

buffer* buffer_alloc(size_t capacity) {
    u_int8_t* new_buf = b_malloc(capacity * sizeof(u_int8_t));
    memset(new_buf, 0, capacity * sizeof(u_int8_t));

    buffer* b = b_malloc(sizeof(buffer));
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
    //     u_int8_t* new_buf = b_malloc(capacity * sizeof(u_int8_t));
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
        b_free(b->buf);
    }
    
    b_free(b);
}

void reverse(u_int8_t* bs, size_t size) {
    int i, j;
    for (i = 0, j = size - 1; i < j; i++, j--) {
        u_int8_t tmp = *(bs + i);
        *(bs + i) = *(bs + j);
        *(bs + j) = tmp;
    }
}

buffer* encode_number(u_int64_t number) {
    u_int8_t nb[16]; 
    int i = 0;
    while (number > 0) {
        nb[i] = number & 0xFF;
        number >>= 8;
        i++;
    }

    reverse(nb, i);

    return buffer_new(nb, i);
}

u_int64_t decode_number(u_int8_t* buf, size_t buf_size) {
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