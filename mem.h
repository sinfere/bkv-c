#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#if !defined(MEM_H)
#define MEM_H

// two buffer type, 32byte / 128byte

#define BKV_BS_DEBUG 1

#define BKV_BUFFER_32_COUNT 64
#define BKV_BUFFER_128_COUNT 8

u_int8_t bkv_mem_32[32 * BKV_BUFFER_32_COUNT];
u_int8_t bkv_mem_128[128 * BKV_BUFFER_128_COUNT];

void bs_init();
void *bs_malloc(size_t size);
void bs_free(void *ptr);
void bs_debug();

#endif