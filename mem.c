#include "mem.h"

// first byte for flag
void bs_init() {
    int i = 0;
    for (i = 0; i < BKV_BUFFER_32_COUNT * 32; i++) {
        *(bkv_mem_32 + i) = 0x00;
        if (i % 32 == 0) {
            *(bkv_mem_32 + i) = 0x10;
        } 
    }

    for (i = 0; i < BKV_BUFFER_128_COUNT * 128; i++) {
        *(bkv_mem_128 + i) = 0x00;
        if (i % 128 == 0) {
            *(bkv_mem_128 + i) = 0x20;
        }
    }    
}

void *bs_malloc(size_t size) {
    u_int8_t* p = NULL;
    int buffer_size = 0;
    int max_count = 0;
    int i = 0;

    if (size < 32) {
        p = bkv_mem_32;
        buffer_size = 32;
        max_count = BKV_BUFFER_32_COUNT;
    } else if (size < 256) {
        p = bkv_mem_128;
        buffer_size = 256;
        max_count = BKV_BUFFER_128_COUNT;
    } else {
        #ifdef BKV_BS_DEBUG
        LOGE("bs_malloc: size not support: %d", (int)size);
        #endif
        
        return NULL;
    }

    for (i = 0; i < max_count; i++) {
        u_int8_t flag = *(p + i * buffer_size);
        if ((flag & 0x0F) == 0) {
            *(p + i * buffer_size) |= 0x01;
            
            #ifdef BKV_BS_DEBUG
            LOGI("bs_malloc: %d / %d", (int)size, i);
            #endif
            
            return p + i * buffer_size + 1;
        }
    }

    #ifdef BKV_BS_DEBUG
    LOGE("bs_malloc fail: %d", (int)size);
    #endif
    
    return NULL;
}


void bs_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    u_int8_t* p = (u_int8_t*) ptr;
    *(p - 1) &= 0xF0;

    #ifdef BKV_BS_DEBUG
    LOGI("bs_free: %p", p);
    #endif    
}

void bs_debug() {
    int i = 0;

    int buffer_32_used_count = 0;
    for (i = 0; i < BKV_BUFFER_32_COUNT; i++) {
        u_int8_t flag = *(bkv_mem_32 + i * 32);
        if ((flag & 0x0F) != 0) {
            buffer_32_used_count++;
        }
    }  

    int buffer_128_used_count = 0;
    for (i = 0; i < BKV_BUFFER_128_COUNT; i++) {
        u_int8_t flag = *(bkv_mem_128 + i * 128);
        if ((flag & 0x0F) != 0) {
            buffer_128_used_count++;
        }
    }       

    LOGI("mem stat: %d / %d", buffer_32_used_count, buffer_128_used_count);  
}