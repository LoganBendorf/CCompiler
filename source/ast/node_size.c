
#include "node_size.h"

#include "logger.h"
#include "helpers.h"

#include <stdio.h>


string get_node_string(const size_t index);


extern bool DEBUG;
extern bool TEST;


size_t size_of_node(const node_type type) {
    switch (type) {
    case RETURN_STATEMENT_NODE: return allign(sizeof(return_statement_node)); break;
    case INTEGER_CONSTANT_NODE: return allign(sizeof(integer_constant_node)); break;
    case PROGRAM_NODE:          return allign(sizeof(program_node));          break;
    case IDENTIFIER_NODE:       return allign(sizeof(identifier_node));       break;
    case TYPE_NODE:             return allign(sizeof(type_node));             break;
    case FUNCTION_NODE:         return allign(sizeof(function_node));         break;
    case UNARY_OP_NODE:         return allign(sizeof(unary_op_node));         break;
    case BINARY_OP_NODE:        return allign(sizeof(binary_op_node));        break;

    default: 
        FATAL_ERROR_STACK_TRACE("Node (%.*s) does not have size available", (int) get_node_string(type).size, get_node_string(type).data);
        return 0;
    }
}







