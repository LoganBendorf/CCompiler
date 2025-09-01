#pragma once

#include "logger.h"

#include <stdio.h>
#include <string.h>

#define LIB_ASSERT_BUF_SIZE 128


#define lib_assert(a, e)                 \
_Generic((a),                            \
    int:     lib_assert_int,             \
    size_t:  lib_assert_size_t,          \
    double:  lib_assert_double,          \
    char:    lib_assert_char,            \
    default: lib_assert_unsupported_type \
)(a, e, #a, #e)

void lib_assert_unsupported_type(void) {
    FATAL_ERROR_STACK_TRACE("lib_assert used with unsupported type\n");
}

void lib_assert_int(const int a, const int e, const char* a_str, const char* e_str) {
    if (a != e) {        
        static char buf[128];
        const int read = snprintf(buf, LIB_ASSERT_BUF_SIZE, "Assertion '%s' == '%s' failed.", a_str, e_str);                        
        const char padding[64] = "                                                                ";
        int pad_size = 12 - read;                                                    
        if (pad_size < 0) { pad_size = 0; }                                                         
        printf("%.*s%.*s Expected %d, got %d.\n", read, buf, pad_size, padding, e, a);                    
    }    
}

void lib_assert_size_t(const size_t a, const size_t e, const char* a_str, const char* e_str) {
    if (a != e) {        
        static char buf[128];
        const int read = snprintf(buf, LIB_ASSERT_BUF_SIZE, "Assertion '%s' == '%s' failed.", a_str, e_str);                        
        const char padding[64] = "                                                                ";
        int pad_size = 12 - read;                                                    
        if (pad_size < 0) { pad_size = 0; }                                                         
        printf("%.*s%.*s Expected %zu, got %zu.\n", read, buf, pad_size, padding, e, a);                    
    }    
}

void lib_assert_double(const double a, const double e, const char* a_str, const char* e_str) {
    if (a != e) {                                                                                   
        static char buf[128];
        const int read = snprintf(buf, LIB_ASSERT_BUF_SIZE, "Assertion '%s' == '%s' failed.", a_str, e_str);                        
        const char padding[64] = "                                                                ";
        int pad_size = 12 - read;                                                    
        if (pad_size < 0) { pad_size = 0; }                                                          
        printf("%.*s%.*s Expected %f, got %f.\n", read, buf, pad_size, padding, e, a);                    
    }    
}


void lib_assert_char(const char a, const char e, const char* a_str, const char* e_str) {
    if (a != e) {                                                                                   
        static char buf[128];
        const int read = snprintf(buf, LIB_ASSERT_BUF_SIZE, "Assertion '%s' == '%s' failed.", a_str, e_str);                        
        const char padding[64] = "                                                                ";
        int pad_size = 12 - read;                                                    
        if (pad_size < 0) { pad_size = 0; }                                                          
        printf("%.*s%.*s Expected %c, got %c.\n", read, buf, pad_size, padding, e, a);                    
    }    
}
















