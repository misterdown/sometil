#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "arena_allocator.h"
#include "utf8_util.h"
#include "ifstream.h"
#include "general.h"
#include "dynamic_string.h"

typedef enum {
    OUTPUT_RAW,
    OUTPUT_HEX,
    OUTPUT_ASCII,
    OUTPUT_TEXTONLY,s
} output_mode_t;

#define BUFFER_SIZE (128)
#define SEARCH_PATTERN_MAX_SIZE (256)
#define DEFAULT_CHUNK_SIZE (1024 * 1024)

static arena_allocator temp_arena = {0};

typedef struct search_ctx {
    const char* pattern;
    size_t pattern_size;
    size_t pattern_utf8_len;
    const char* filepath;
    ifstream* stream;
    dstring line_buffer;
    size_t current_line;
    size_t current_char;
    bool timing;
    bool counting;
    bool printing;
    bool grep_mode;
} search_ctx;

typedef struct print_ctx {
    output_mode_t mode;
    size_t bytes_per_line;
} print_ctx;


void print_usage(const char* prog_name) {
    printf("Usage: %s <filename> [options]\n", prog_name);
    printf("\nBasic options:\n");
    printf("  -h/--help      Show this message\n");
    printf("  -s <str>       Search for text pattern\n");
    printf("  -x <hex>       Search for hex pattern (e.g. \"DEADBEEF\")\n");
    printf("  -v <mode>      View file content with specified mode\n");
    printf("  -w <num>       Bytes per line (default: 16, only with -v)\n");
    printf("\nOutput control:\n");
    printf("  -np            Disable printing of matches (only count)\n");
    printf("  -nc            Disable match counting\n");
    printf("  -t             Enable timing measurements\n");
    printf("\nView modes (-v option):\n");
    printf("  raw            Raw byte output (default)\n");
    printf("  hex            Hexadecimal dump\n");
    printf("  ascii          Combined hex/ASCII view\n");
    printf("  textonly       Text only output (UTF-8 aware)\n");
    printf("\nExamples:\n");
    printf("  %s file.txt -s \"pattern\"      Search for text pattern\n", prog_name);
    printf("  %s file.bin -x \"C0FFEE\" -t    Search hex with timing\n", prog_name);
    printf("  %s file.txt -v hex -w 32      View as hex dump (32 bytes/line)\n", prog_name);
    printf("  %s file.txt -s \"text\" -np     Search without printing matches\n", prog_name);
}

output_mode_t parse_output_mode(const char* mode_str) {
    assert(mode_str != NULL);

    if (strcmp(mode_str, "hex") == 0) return OUTPUT_HEX;
    if (strcmp(mode_str, "ascii") == 0) return OUTPUT_ASCII;
    if (strcmp(mode_str, "textonly") == 0) return OUTPUT_TEXTONLY;
    return OUTPUT_RAW;
}

bool parse_hex_pattern(const char* str, size_t max_str_size, char* out, size_t* out_len) {
    assert(str != NULL);
    assert(out != NULL);
    assert(out_len != NULL);
    assert(max_str_size != 0);

    size_t len = 0;
    const char* p = str;
    
    while ((max_str_size > len) && *p) {
        while (*p && !isxdigit(*p)) p++;
        if (!*p) break;
        
        if (!*(p + 1) || !isxdigit(*(p + 1))) return false;
        
        if (sscanf_s(p, "%2hhx", &out[len++]) != 1) {
            return false;
        }
        p += 2;
    }
    
    *out_len = len;
    return (len > 0);
}
size_t sprint_hex_byte(char* data, unsigned char byte) {
    assert(data != NULL);
    
    static const char hex_digits[] = "0123456789ABCDEF";
    
    data[0] = hex_digits[(byte >> 4) & 0x0F];  // H
    data[1] = hex_digits[byte & 0x0F];         // L
    
    return 2;
}

void print_data(const char* data, size_t size, size_t bytes_per_line, output_mode_t mode) {
    char buffer[BUFFER_SIZE];
    size_t buffer_pos = 0;
    bool last_printable = true;

    memset(buffer, 0, BUFFER_SIZE);

    for (size_t i = 0; i < size; ++i) {
        switch (mode) {
            case OUTPUT_HEX: {
                buffer_pos += sprint_hex_byte(&buffer[buffer_pos], (unsigned char)data[i]);
                buffer[buffer_pos++] = ' ';
                break;
            }
            case OUTPUT_ASCII: {
                bool printable = isascii(data[i]) && !isspace(data[i]) && !iscntrl(data[i]);
                
                buffer_pos += sprint_hex_byte(&buffer[buffer_pos], (unsigned char)data[i]);
                buffer[buffer_pos++] = printable ? data[i] : '.';
                buffer[buffer_pos++] = ' ';
                break;
            }
            
            case OUTPUT_TEXTONLY: {
                bool printable = isascii(data[i]) && !isspace(data[i]) && !iscntrl(data[i]);
                
                if (last_printable && !printable) {
                    buffer[buffer_pos++] = ' ';
                } else if (printable) {
                    buffer[buffer_pos++] = data[i];
                }
                last_printable = printable;
                break;
            }
            
            default:
            case OUTPUT_RAW: {
                bool printable = isascii(data[i]) && !isspace(data[i]) && !iscntrl(data[i]);
                buffer[buffer_pos++] = printable ? data[i] : '.';
                buffer[buffer_pos++] = ' ';
                break;
            }
        }

        
        if ((i % bytes_per_line == 0) && (i != 0)) {
            if (mode != OUTPUT_TEXTONLY) {
                // buffer[buffer_pos++] = '\n';
                // fwrite(buffer, buffer_pos, 1, stdout);
                // buffer_pos = 0;
            }
        }
        if (buffer_pos >= (BUFFER_SIZE - 32)) {
            fwrite(buffer, buffer_pos, 1, stdout);
            buffer_pos = 0;
        }
    }
    
    if (buffer_pos > 0) {
        fwrite(buffer, 1, buffer_pos, stdout);
    }
    printf("\n");
    fflush(stdout);
}

void search_file(search_ctx* ctx) {
    if (ctx->pattern_size == 0) {
        fprintf(stderr, "Empty search pattern\n");
        exit(EXIT_FAILURE);
    }
    arena_slice slice = arena_push(&temp_arena, ctx->pattern_size, 8);
    char* window = slice.allocated;

    clock_t start_time = clock();

    size_t window_pos = 0;
    int ch = 0;
    size_t char_in_line = 0;
    size_t line_number = 0;
    size_t counter = 0;
    size_t char_counter = 0;
    bool match_this_line = false;
    while ((ch = ifstream_getc(ctx->stream)) != EOF) {
        window[window_pos] = (char)ch;
        window_pos = (window_pos + 1) % ctx->pattern_size;
        
        bool match = true;
        for (size_t i = 0; i < ctx->pattern_size; i++) {
            size_t idx = (window_pos + i) % ctx->pattern_size;
            if (window[idx] != ctx->pattern[i]) {
                match = false;
                break;
            }
        }
        if (ctx->grep_mode && ctx->printing && ch != '\n')
            dstring_append_char(&ctx->line_buffer, (char)ch);
        match_this_line = match_this_line || match;
        
        if (match) {
            ++counter;
            const size_t match_pos = char_in_line - ctx->pattern_utf8_len + 1;
            if (ctx->printing && !ctx->grep_mode) {
                printf("%s:%zu:%zu\n", ctx->filepath, line_number + 1, match_pos + 1);
            }
        }

        if (char_counter == 0) {
            if (ch == '\n') {
                char_in_line = 0;
                if (ctx->grep_mode && ctx->printing) {
                    if (match_this_line)
                        printf("%zu:%s\n", line_number + 1, dstring_cstr(&ctx->line_buffer));
                    dstring_clear(&ctx->line_buffer);
                }
                ++line_number;
                match_this_line = false;
            } else {
                ++char_in_line;
                char_counter = utf8_length[ch];
            }
            
            // if ((ch & 0xC0) != 0x80) char_in_line++; // A bit slower for some reason
        }
        if (char_counter == 0) {
            char_counter = 1;
        }
        --char_counter;

    }

    clock_t end_time = clock();
    double elapsed_sec = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    if (ctx->counting) {
        printf("Total matches: %zu\n", counter);
    }
    if (ctx->timing) {
        printf("Search time: %.3lf seconds\n", elapsed_sec);
    }
    arena_pop(slice);
}
void print_file(print_ctx* ctx, ifstream* stream) {
    char buffer[BUFFER_SIZE];
    int ch = 0;
    size_t printed = 0;

    memset(buffer, 0, BUFFER_SIZE);

    while ((ch = ifstream_getc(stream)) != EOF) {
        size_t buffer_pos = 0;
        bool last_printable = true;
        buffer[0] = '\0';

        switch (ctx->mode) {
            case OUTPUT_HEX: {
                buffer_pos += sprint_hex_byte(&buffer[buffer_pos], (unsigned char)ch);
                buffer[buffer_pos++] = ' ';
                break;
            }
            case OUTPUT_ASCII: {
                bool printable = isascii(ch) && !isspace(ch) && !iscntrl(ch);
                
                buffer_pos += sprint_hex_byte(&buffer[buffer_pos], (unsigned char)ch);
                buffer[buffer_pos++] = printable ? ch : '.';
                buffer[buffer_pos++] = ' ';
                break;
            }
            
            case OUTPUT_TEXTONLY: {
                bool printable = isascii(ch) && !isspace(ch) && !iscntrl(ch);
                
                if (last_printable && !printable) {
                    buffer[buffer_pos++] = ' ';
                } else if (printable) {
                    buffer[buffer_pos++] = ch;
                }
                last_printable = printable;
                break;
            }
            
            default:
            case OUTPUT_RAW: {
                bool printable = isascii(ch) && !isspace(ch) && !iscntrl(ch);
                buffer[buffer_pos++] = printable ? ch : '.';
                buffer[buffer_pos++] = ' ';
                break;
            }
        }

        
        if ((printed % ctx->bytes_per_line == 0) && (printed != 0)) {
            if (ctx->mode != OUTPUT_TEXTONLY) {
                buffer[buffer_pos++] = '\n';
            }
        }
        if (buffer_pos >= (BUFFER_SIZE - 32)) {
            fwrite(buffer, buffer_pos, 1, stdout);
            buffer_pos = 0;
        }
        
        if (buffer_pos > 0) {
            fwrite(buffer, 1, buffer_pos, stdout);
        }
        ++printed;
    }
    printf("\n");
    fflush(stdout);
}

void cleanup() {
    arena_drop(&temp_arena);
}

int main(int argc, char** argv) {
    atexit(cleanup);

    if (!setlocale(LC_CTYPE, "en_US.UTF-8")) {
        if (!setlocale(LC_CTYPE, "en_US.utf8")) {
            if (!setlocale(LC_CTYPE, "C.UTF-8")) {
                fprintf(stderr, "Failed to set locale, UTF-8 support may be limited\n");
                setlocale(LC_CTYPE, "");
            }
        }
    }

    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    output_mode_t mode = OUTPUT_RAW;
    size_t bytes_per_line = 16;
    const char* filename = NULL;
    bool search_mode = false;
    bool is_view_mode = false;
    bool counting = true;
    bool timing = false;
    bool printing = true;
    bool grep_mode = false;
    char search_pattern[SEARCH_PATTERN_MAX_SIZE] = {0};
    size_t search_pattern_size = 0;

    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
            print_usage(argv[0]);
            
        } if (strcmp(argv[i], "-np") == 0) {
            printing = false;
            
        } else if (strcmp(argv[i], "-nc") == 0) {
            counting = false;
            
        } else if (strcmp(argv[i], "-t") == 0) {
            timing = true;
            
        } else if (strcmp(argv[i], "-g") == 0) {
            grep_mode = true;
            
        } else if (strcmp(argv[i], "-w") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Missing argument for -w\n");
                return EXIT_FAILURE;
            }
            bytes_per_line = strtoul(argv[i], NULL, 10);
            if (bytes_per_line == 0 || bytes_per_line > 1024) {
                fprintf(stderr, "Invalid bytes per line value\n");
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Missing argument for -v\n");
                return EXIT_FAILURE;
            }
            if (search_mode) {
                fprintf(stderr, "Cannot combine view and search options\n");
                return EXIT_FAILURE;
            }
            mode = parse_output_mode(argv[i]);
            is_view_mode = true;
            
        } else if (strcmp(argv[i], "-s") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Missing pattern for -s\n");
                return EXIT_FAILURE;
            }
            if (is_view_mode) {
                fprintf(stderr, "Cannot combine view and search options\n");
                return EXIT_FAILURE;
            }
            if (strncpy_s(search_pattern, sizeof(search_pattern), argv[i], _TRUNCATE) != 0) {
                PRINT_ERRNO("Failed to copy search pattern");
                return EXIT_FAILURE;
            }
            search_pattern[sizeof(search_pattern) - 1] = '\0';
            search_pattern_size = strlen(search_pattern);
            search_mode = true;

        } else if (strcmp(argv[i], "-x") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Missing hex pattern for -x\n");
                return EXIT_FAILURE;
            }
            if (!parse_hex_pattern(argv[i], sizeof(search_pattern), search_pattern, &search_pattern_size)) {
                fprintf(stderr, "Invalid hex pattern\n");
                return EXIT_FAILURE;
            }
            if (is_view_mode) {
                fprintf(stderr, "Cannot combine view and search options\n");
                return EXIT_FAILURE;
            }
            search_mode = true;
        } else if (argv[i][0] != '-') {
            if (!filename)
                filename = argv[i];
        }
    }

    if (!filename) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    FILE* file = NULL;
    if (fopen_s(&file, filename, "rb") != 0) {
        PRINT_ERRNO("Failed to open file");
        return EXIT_FAILURE;
    }

    arena_allocator alloc = {0};
    ifstream stream = {0};
    ifstream_init(&stream, file);

    if (search_mode) {
        search_ctx ctx = {
            .pattern = search_pattern,
            .pattern_size = search_pattern_size,
            .pattern_utf8_len = utf8_strlen(search_pattern, search_pattern_size),
            .filepath = filename,
            .line_buffer = dstring_new(&alloc),
            .stream = &stream,
            .current_line = 0,
            .current_char = 0,
            .timing = timing,
            .counting = counting,
            .printing = printing,
            .grep_mode = grep_mode,
        };

        search_file(&ctx);
    } else {
        print_ctx ctx = { mode, bytes_per_line };
        print_file(&ctx, &stream);
    }

    ifstream_close(&stream);
    fclose(file);
    if (file && ferror(file)) {
        PRINT_ERRNO("Error closing file");
    }
    arena_drop(&alloc);

    return EXIT_SUCCESS;
}