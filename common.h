#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(COMMON_H)
#define COMMON_H

#define USE_TLSF 1

#ifdef USE_TLSF
    #include "tlsf.h"

    #define TLSF_POOL_SIZE 3 * 1024
    static char b_mem_pool[TLSF_POOL_SIZE];
#endif


void b_init();
void* b_malloc(size_t size);
void* b_realloc(void* p, size_t size);
void b_free(void* p);




#define BUFFER_DEFAULT_SIZE 64

typedef struct {
    u_int8_t* buf;
    size_t size;
    size_t capacity;
} buffer;

buffer* buffer_new(u_int8_t* buf, size_t size);
buffer* buffer_new_from_number(u_int64_t n);
buffer* buffer_new_from_string(char* s);
buffer* buffer_alloc(size_t capacity);
buffer* buffer_clone(buffer* b);
int buffer_grow(buffer* b, size_t capacity);
int buffer_append(buffer* b, u_int8_t* buf, size_t size);
void buffer_free(buffer* b);

void reverse(u_int8_t* bs, size_t size);
buffer* encode_number(u_int64_t number);
u_int64_t decode_number(u_int8_t* buf, size_t buf_size);







#define USE_TTY 1
#define TIME_FORMAT "%F %T"

#define LOGI(format, ...)                                                    \
    do {                                                                     \
        time_t now = time(NULL);                                             \
        char timestr[20];                                                    \
        strftime(timestr, 20, TIME_FORMAT, localtime(&now));                 \
        if (USE_TTY) {                                                       \
            fprintf(stdout, "\e[01;32m%s INFO: \e[0m" format "\n", timestr, \
                    ## __VA_ARGS__);                                         \
            fflush(stdout);                                                  \
        } else {                                                             \
            fprintf(stdout, "%s INFO: " format "\n", timestr,               \
                    ## __VA_ARGS__);                                         \
            fflush(stdout);                                                  \
        }                                                                    \
    }                                                                        \
    while (0)

#define LOGE(format, ...)                                                     \
    do {                                                                      \
        time_t now = time(NULL);                                              \
        char timestr[20];                                                     \
        strftime(timestr, 20, TIME_FORMAT, localtime(&now));                  \
        if (USE_TTY) {                                                        \
            fprintf(stderr, "\e[01;35m%s ERROR: \e[0m" format "\n", timestr, \
                    ## __VA_ARGS__);                                          \
            fflush(stderr);                                                   \
        } else {                                                              \
            fprintf(stderr, "%s ERROR: " format "\n", timestr,               \
                    ## __VA_ARGS__);                                          \
            fflush(stderr);                                                   \
        }                                                                     \
    }                                                                         \
    while (0)


#endif