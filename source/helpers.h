#pragma once

#include "structs.h"

#include <stdint.h>



#define GLOBAL_STACK_ALLIGNMENT 8

// Maybe should go in macros idk
#define ITER_NEXT(it) \
    ((it).next(&(it)))



[[nodiscard]] int atoin(const string str);
__attribute__((__used__)) void debug_print_ast_node(const uint8_t* addr);

[[nodiscard]] size_t allign(const size_t size);










