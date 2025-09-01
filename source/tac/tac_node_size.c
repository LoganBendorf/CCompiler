
#include "tac_node_size.h"

#include "logger.h"
#include "helpers.h"

#include <stdio.h>


string get_tac_node_string(const size_t index);


extern bool DEBUG;
extern bool TEST;


size_t size_of_tac_node(const tac_node_type type) {
    switch (type) {
    case FUNCTION_TAC_NODE:     return allign(sizeof(function_tac_node));     break;
    case PROGRAM_TAC_NODE:      return allign(sizeof(program_tac_node));      break;
    case INT_CONSTANT_TAC_NODE: return allign(sizeof(int_constant_tac_node)); break;
    case VAR_TAC_NODE:          return allign(sizeof(var_tac_node));          break;
    case RETURN_TAC_NODE:       return allign(sizeof(return_tac_node));       break;
    case UNARY_OP_TAC_NODE:     return allign(sizeof(unary_op_tac_node));     break;
    case BINARY_OP_TAC_NODE:    return allign(sizeof(binary_op_tac_node));    break;

    default: 
        FATAL_ERROR_STACK_TRACE("Asm Node (%.*s) does not have size available", (int) get_tac_node_string(type).size, get_tac_node_string(type).data);
        return 0;
    }
}






