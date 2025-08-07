#pragma once

#include "structs.h"

typedef enum {
    NEGATE_OP, BITWISE_COMPLEMENT_OP, PRE_DECREMENT_OP, POST_DECREMENT_OP
} unary_op_type;

[[nodiscard]] string get_unary_op_string(const size_t index);





