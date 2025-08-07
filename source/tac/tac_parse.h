#pragma once

#include "node.h"
#include "tac_node.h"



typedef struct {
    const bool is_error;
    union {
        const program_tac_node node;
        const string       error_msg;
    };
} expect_program_tac_node;
    

// Should just return void? Later functions will check instruction stack
[[nodiscard]] expect_program_tac_node parse_ast(const program_node program);









