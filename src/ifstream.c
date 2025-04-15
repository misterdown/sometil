#include <assert.h>
#include <stdlib.h>

#include "utf8_util.h"
#include "ifstream.h"

#define IFSTREAM_BUFFER_SIZE (1 << 20) // 1 MB буфер

void ifstream_init(ifstream* stream, FILE* file) {
    assert(stream != NULL);
    assert(file != NULL);

    stream->file = file;
    stream->buffer = calloc(1, IFSTREAM_BUFFER_SIZE);
    stream->pos = 0;
    stream->size = 0;
    stream->eof = false;
    stream->total_read = 0;
}

void ifstream_close(ifstream* stream) {
    if (stream && stream->buffer) {
        free(stream->buffer);
        stream->buffer = NULL;
    }
}

int ifstream_getc(ifstream* stream) {
    assert(stream != NULL);
    assert(stream->file != NULL);

    if (stream->pos >= stream->size) {
        stream->size = fread(stream->buffer, 1, IFSTREAM_BUFFER_SIZE, stream->file);
        stream->total_read += stream->size;
        stream->pos = 0;
        
        if (stream->size == 0) {
            stream->eof = true;
            return EOF;
        }
    }
    
    return (unsigned char)stream->buffer[stream->pos++];
}

int32_t ifstream_getc_utf8(ifstream* stream) {

    int first_byte = ifstream_getc(stream);
    if (first_byte == EOF) return EOF;

    const uint8_t byte = (uint8_t)first_byte;
    const int bytes_total = utf8_length[byte];
    
    if (bytes_total == 0) return -1;
    if (bytes_total == 1) return byte; // ASCII

    int32_t codepoint = byte & (0x7F >> bytes_total);
    for (int i = 1; i < bytes_total; i++) {
        int next_byte = ifstream_getc(stream);
        if (next_byte == EOF) return EOF;
        if ((next_byte & 0xC0) != 0x80) return -1;
        
        codepoint = (codepoint << 6) | (next_byte & 0x3F);
    }

    // Overlong encoding
    if ((bytes_total == 2 && codepoint < 0x80) ||
        (bytes_total == 3 && codepoint < 0x800) ||
        (bytes_total == 4 && codepoint < 0x10000)) {
        return -1;
    }

    if (codepoint > 0x10FFFF) return -1;
    
    return codepoint;
}