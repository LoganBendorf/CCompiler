#pragma once

#include "tac_node.h"
#include "asm_node.h"

typedef struct {
    const bool is_error;
    union {
        const program_asm_node node;
        const string            error_msg;
    };
} expect_program_asm_node;


[[nodiscard]] expect_program_asm_node codegen(const program_tac_node program);









