#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>  // For backtrace
#include <unistd.h>


void print_stack_trace();

const char* GREEN();
const char* YELLOW();
const char* RED();
const char* RESET();


#define FATAL_ERROR(fmt, ...) \
    do { \
        fprintf(stderr, "FATAL ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
        exit(EXIT_FAILURE); \
    } while(0)

    

#define FATAL_ERROR_STACK_TRACE(fmt, ...) \
    do { \
        fprintf(stderr, "FATAL ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
        print_stack_trace(); \
        exit(EXIT_FAILURE); \
    } while(0)


