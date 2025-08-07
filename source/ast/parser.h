#pragma once

#include "node.h"


typedef struct {
    const bool is_error;
    union {
        const program_node node;
        const string       error_msg;
    };
} expect_program_node;



[[nodiscard]] expect_program_node parse();


