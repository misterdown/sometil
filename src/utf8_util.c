#include "utf8_util.h"

size_t utf8_strlen(const char* str, size_t max_bytes) {
    if (str == NULL) return (size_t)-1;
    
    size_t char_count = 0;
    size_t i = 0;
    
    while (1) {
        if (max_bytes > 0 && i >= max_bytes) break;
        if (max_bytes == 0 && str[i] == '\0') break;
        
        uint8_t c = (uint8_t)str[i];
        
        if (isascii(c)) {
            if (c == '\0' && max_bytes == 0) break;
            i += 1;
            char_count++;
            continue;
        }
        
        uint8_t char_len = utf8_length[c];
        if (char_len == 0) {
            return -1;
        }
        
        if (max_bytes > 0 && i + char_len > max_bytes) return (size_t)-1;
        
        for (size_t j = 1; j < char_len; j++) {
            if ((str[i+j] & 0xC0) != 0x80) return (size_t)-1;
        }
        
        uint32_t codepoint = 0;
        switch (char_len) { 
            case 2:
                codepoint = ((c & 0x1F) << 6) | (str[i+1] & 0x3F);
                if (codepoint < 0x80) return (size_t)-1;
                break;
            case 3:
                codepoint = ((c & 0x0F) << 12) | ((str[i+1] & 0x3F) << 6) | (str[i+2] & 0x3F);
                if (codepoint < 0x800) return (size_t)-1;
                break;
            case 4:
                codepoint = ((c & 0x07) << 18) | ((str[i+1] & 0x3F) << 12) | ((str[i+2] & 0x3F) << 6) | (str[i+3] & 0x3F);
                if (codepoint < 0x10000) return (size_t)-1;
                if (codepoint > 0x10FFFF) return (size_t)-1;
                break;
        }
        
        i += char_len;
        char_count++;
    }
    
    return char_count;
}