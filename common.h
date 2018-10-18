#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(COMMON_H)
#define COMMON_H

#define BUFFER_DEFAULT_SIZE 64

typedef struct {
    u_int8_t* buf;
    size_t size;
    size_t capacity;
} buffer;

buffer* buffer_new(u_int8_t* buf, size_t size);
buffer* buffer_alloc(size_t capacity);
buffer* buffer_clone(buffer* b);
int buffer_grow(buffer* b, size_t capacity);
int buffer_append(buffer* b, u_int8_t* buf, size_t size);
void buffer_free(buffer* b);

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