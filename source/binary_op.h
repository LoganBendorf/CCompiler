#pragma once

#include "structs.h"

typedef enum {
    ADD_OP, SUB_OP, MUL_OP, DIV_OP, MOD_OP
} binary_op_type;

[[nodiscard]] string get_binary_op_string(const size_t index);













