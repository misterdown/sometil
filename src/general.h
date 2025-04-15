#ifndef __GENERAL_H__
#define __GENERAL_H__ 1

#include <stdio.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define PRINT_ERRNO(msg) \
    do { \
        fprintf(stderr, "[%s:%s] %s: ", __FILE__, TOSTRING(__LINE__), msg); \
        perror(""); \
    } while(0)

#endif // __GENERAL_H__