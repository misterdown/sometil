#ifndef __IFSTREAM_H__
#define __IFSTREAM_H__ 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct ifstream {
    FILE* file;
    char* buffer;
    size_t pos;
    size_t size;
    bool eof;
    size_t total_read;
} ifstream;


void ifstream_init(ifstream *stream, FILE* file);
void ifstream_close(ifstream* stream);

int ifstream_getc(ifstream* stream);
int32_t ifstream_getc_utf8(ifstream* stream);
#endif // __IFSTREAM_H__