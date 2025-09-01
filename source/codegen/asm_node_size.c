
#include "asm_node_size.h"

#include "logger.h"
#include "helpers.h"

#include <stdio.h>


string get_asm_node_string(const size_t index);



extern bool DEBUG;
extern bool TEST;



size_t size_of_asm_node(const asm_node_type type) {
    switch (type) {
    case FUNCTION_ASM_NODE:      return allign(sizeof(function_asm_node));      break;
    case INT_IMMEDIATE_ASM_NODE: return allign(sizeof(int_immediate_asm_node)); break;
    case RETURN_ASM_NODE:        return allign(sizeof(return_asm_node));        break;
    case PROGRAM_ASM_NODE:       return allign(sizeof(program_asm_node));       break;
    case REGISTER_ASM_NODE:      return allign(sizeof(register_asm_node));      break;
    case UNARY_OP_ASM_NODE:      return allign(sizeof(unary_op_asm_node));      break;
    case BLOCK_ASM_NODE:         return allign(sizeof(block_asm_node));         break;
    case TEMP_REG_ASM_NODE:      return allign(sizeof(temp_reg_asm_node));      break;
    case MOVE_ASM_NODE:          return allign(sizeof(move_asm_node));          break;
    case ASM_NODE_LINKED_PTR:    return allign(sizeof(asm_node_linked_ptr));    break;

    default: 
        FATAL_ERROR_STACK_TRACE("Asm Node (%.*s) does not have size available", (int) get_asm_node_string(type).size, get_asm_node_string(type).data);
        return 0;
    }
}






