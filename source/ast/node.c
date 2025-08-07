

#include "node.h"

#include "logger.h"
#include "token.h"
#include "helpers.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>



extern bool DEBUG;
extern bool TEST;



static size_t prev_stack_push_size;



static size_t node_addr_to_str(const uint8_t* addr, char* buf, const size_t buf_size, size_t index);
static string type_node_to_str(const type_node nd);

#define BUFFER_SIZE 1024 * 8


string_span node_span() {
    static const string arr[] = {
                     {"NULL_NODE", 9}, {"PROGRAM_NODE", 12}, {"FUNCTION_NODE", 13}, {"IDENTIFIER_NODE", 15}, {"BLOCK_STATEMENT_NODE", 20}, {"STATEMENT_NODE", 14}, {
                        "RETURN_STATEMENT_NODE", 21}, {"INTEGER_CONSTANT_NODE", 21}, {"IF_STATEMENT_NODE", 18},
               {"CUSTOM_TYPE_NODE", 16}, {"C_TYPE_NODE", 11}, {"TYPE_NODE", 9}, {"UNARY_OP_NODE", 13}, {"BINARY_OP_NODE", 14}
                    };
    return (string_span){arr, sizeof(arr) / sizeof(arr[0]) };
}

string get_node_string(const size_t index) {

    const string_span node_strs = node_span();

    if (index >= node_strs.size) {
        FATAL_ERROR_STACK_TRACE("get_node_string(): Out of bounds index, (%zu)", index); }

    return node_strs.strs[index];
}



node_stack g_node_stack = {8,0, {0}};

static size_t allign(const size_t size) {
    const size_t to_add = size % g_node_stack.allignment;
    return size + to_add;
}

static size_t size_of_node(const node_type type) {
    switch (type) {
    case RETURN_STATEMENT_NODE: return allign(sizeof(return_statement_node)); break;
    case INTEGER_CONSTANT_NODE: return allign(sizeof(integer_constant_node)); break;
    case PROGRAM_NODE:          return allign(sizeof(program_node));          break;
    case IDENTIFIER_NODE:       return allign(sizeof(identifier_node));       break;
    case TYPE_NODE:             return allign(sizeof(type_node));             break;
    case FUNCTION_NODE:         return allign(sizeof(function_node));         break;
    case UNARY_OP_NODE:         return allign(sizeof(unary_op_node));         break;

    default: 
        FATAL_ERROR_STACK_TRACE("Node (%.*s) does not have size available", (int) get_node_string(type).size, get_node_string(type).data);
        return 0;
    }
}

[[maybe_unused]] static size_t size_of_node_at_stack_ptr(const node_stack_ptr ptr) {
    return size_of_node((node_type) *ptr.addr);
}

static size_t size_of_node_at_ptr(const node_ptr ptr) {
    return size_of_node((node_type) *ptr.addr);
}

static node_stack_ptr g_node_stack_top_ptr() {
    const node_stack_ptr addr = {&g_node_stack.stack[g_node_stack.top], false};
    return addr;
}

[[maybe_unused]] static node_stack_ptr pop_g_node_stack() {
    if (g_node_stack.top == 0) {
        const node_stack_ptr ret_val = {0, true};
        return ret_val;
    }

    const size_t size = prev_stack_push_size;
    if (size > g_node_stack.top) {
        FATAL_ERROR_STACK_TRACE("node size to pop is greater than node stack top. Would lead to underflow, bad!!!"); }

    g_node_stack.top -= size;

    return  g_node_stack_top_ptr();
}


node_stack_ptr get_top_of_g_node_stack() {
    if (g_node_stack.top == 0) {
        const node_stack_ptr ret_val = {0, true};
        return ret_val;
    }

    const size_t size = prev_stack_push_size;
    if (size > g_node_stack.top) {
        FATAL_ERROR_STACK_TRACE("Can not get top if global node stack, would lead to underflow. Top = %zu, request size = %zu", g_node_stack.top, size); }

    node_stack_ptr ret = g_node_stack_top_ptr();
    ret.addr -= size;

    return ret;
}

    void push_g_node_stack(const node_ptr ptr) {
    if (ptr.is_none) {
        FATAL_ERROR_STACK_TRACE("Can't push none pointer"); }

    const size_t size = size_of_node_at_ptr(ptr);
    if (size + g_node_stack.top >= NODE_STACK_SIZE) {
        FATAL_ERROR_STACK_TRACE("Stack OOM"); }

    memcpy((void*) g_node_stack_top_ptr().addr,  (void*) ptr.addr, size);
    
    g_node_stack.top += size;

    prev_stack_push_size = size;
}



static string type_node_to_str(const type_node nd) {
    if (nd.is_c_type) {
        return get_token_string(nd.type.c_type.c_token_type);
    } else {
        FATAL_ERROR("Custom types not supported");
        return (string){};
    }
}

static size_t unary_op_node_to_str(const unary_op_node un_op_nd, char* buf, const size_t buf_size, size_t index) {

    const unary_op_type op_type = un_op_nd.op_type;

    // Pre
    bool printed_op = false;
    switch (op_type) {
    case NEGATE_OP:             index += (size_t)snprintf(buf + index, buf_size - index, "-");  printed_op = true; break;
    case BITWISE_COMPLEMENT_OP: index += (size_t)snprintf(buf + index, buf_size - index, "~");  printed_op = true; break;
    case PRE_DECREMENT_OP:      index += (size_t)snprintf(buf + index, buf_size - index, "--"); printed_op = true; break;
    default: break;
    }

    index = node_addr_to_str((const uint8_t*) un_op_nd.expr.addr, buf, buf_size, index);

    // Post
    switch (op_type) {
    case POST_DECREMENT_OP: index += (size_t)snprintf(buf + index, buf_size - index, "--"); printed_op = true; break;
    default: break;
    }

    if (!printed_op) {
        FATAL_ERROR("Cannot print operator of type (%u), yes there's no string for it cope", op_type); }

    return index;
}

// FIXME Use snprintf
static size_t node_addr_to_str(const uint8_t* addr, char* buf, const size_t buf_size, size_t index) {

    const node_type type = *addr;

    const string node_type_str = get_node_string(type);
    
    switch (type) {
    case RETURN_STATEMENT_NODE: { // This is actually the worst thing ever
        const return_statement_node ret_stmt_nd = *((return_statement_node*) addr);
        index += (size_t)snprintf(buf + index, buf_size - index, "return ");
        index = node_addr_to_str(ret_stmt_nd.expr.addr, buf, buf_size, index);
        index += (size_t)snprintf(buf + index, buf_size - index, ";");
    } break;
    case INTEGER_CONSTANT_NODE: {
        const integer_constant_node int_constant_nd = *((integer_constant_node*) addr);
        index += (size_t)snprintf(buf + index, buf_size - index, "%d", int_constant_nd.value);
    } break;
    case IDENTIFIER_NODE: {
        const identifier_node ident_nd = *((identifier_node*) addr);
        index +=(size_t)snprintf(buf + index, buf_size - index, "%.*s", (int) ident_nd.value.size, ident_nd.value.data);
    } break;
    case FUNCTION_NODE: {
        const function_node func_nd         = *((function_node*) addr);
        const string        return_type_str = type_node_to_str(func_nd.return_type);
        const string        name            = func_nd.name;
        const string        param_str       = type_node_to_str(func_nd.parameter);

        index += (size_t)snprintf(buf + index, buf_size - index, "%.*s %.*s(%.*s) {\n", 
            (int) return_type_str.size, return_type_str.data,
            (int) name.size,            name.data,
            (int) param_str.size,       param_str.data
        );

        const block_statement_node body = func_nd.body;
        for (size_t i = 0; i < body.statements.size; i++) {
            const stmt_ptr stmt     = body.statements.values[i];
            index += (size_t)snprintf(buf + index, buf_size - index, "  ");
            index = node_addr_to_str(stmt.addr, buf, buf_size, index);
            index += (size_t)snprintf(buf + index, buf_size - index, "\n");
        }

        index += (size_t)snprintf(buf + index, buf_size - index, "}");

    } break;
    case TYPE_NODE: {
        const type_node type_nd = *((type_node*) addr);
        const string    str     = type_node_to_str(type_nd);
        index += (size_t)snprintf(buf + index, buf_size - index, "%.*s", (int) str.size, str.data);
    } break;
    case PROGRAM_NODE: {
        const program_node prog_nd  = *((program_node*) addr);
        index += (size_t)snprintf(buf + index, buf_size - index, "(Program)\n");
        index = node_addr_to_str((const uint8_t*) &prog_nd.main_function, buf, buf_size, index);
        index += (size_t)snprintf(buf + index, buf_size - index, "\n");
    } break;
    case UNARY_OP_NODE: {
        const unary_op_node un_op_nd  = *((unary_op_node*) addr);
        index = unary_op_node_to_str(un_op_nd, buf, buf_size, index);
    } break;

    default: 
        FATAL_ERROR("Cannot convert (%.*s) node to string", (int) node_type_str.size, node_type_str.data);
        return 0;
    }

    return index;
}

void print_program_ast(const program_node prog) {
    printf("PRINTING PROGRAM AST -------------\n");
    static char buf[BUFFER_SIZE];
    const size_t start_index = 0;
    const size_t end_index = node_addr_to_str((const uint8_t*) &prog, buf, BUFFER_SIZE, start_index);
    printf("%.*s", (int) end_index + 1, buf + start_index);
    printf("DONE -----------------------------\n\n");
}

void print_g_nodes(void) {

    printf("PRINTING AST NODES ---------------\n");

    size_t max_type_width = 0;
    size_t stack_pos = 0;
    size_t num_nodes = 0;
    while (stack_pos < g_node_stack.top) {

        
        const uint8_t* addr = g_node_stack.stack + stack_pos;
        
        const node_type type = *addr;
        
        const string node_type_str = get_node_string(type);
        max_type_width = node_type_str.size > max_type_width ? node_type_str.size : max_type_width;
        
        num_nodes++;

        const size_t jump_size = size_of_node((node_type) *(addr));

        stack_pos += jump_size;
    }

    char str[32];
    sprintf(str, "%zu", num_nodes);
    const size_t max_num_width = strlen(str) + 1; 



    stack_pos = 0;
    size_t count = 1;
    while (stack_pos < g_node_stack.top) {

        const uint8_t* addr = g_node_stack.stack + stack_pos;
        
        const node_type type = *addr;

        const string node_type_str = get_node_string(type);

        static char buf[BUFFER_SIZE];
        const size_t start_index = 0;
        const size_t end_index = node_addr_to_str(addr, buf, BUFFER_SIZE, start_index);

        bool found_newline = false;
        for (size_t i = 0; i < end_index; i++) {
            if (buf[i] == '\n') { found_newline = true; }
        }

        const char* add_newline = found_newline ? "\n" : "";

        const int pad_size = found_newline ? 0 : (int) max_type_width - (int) node_type_str.size;

        printf("%*zu: Type (%.*s)%s%*s'%.*s'\n", 
            (int) max_num_width,      count++, 
            (int) node_type_str.size, node_type_str.data, 
                  add_newline,
                  pad_size,           "", 
            (int) end_index + 1,      buf + start_index
        );

        const size_t jump_size = size_of_node((node_type) *(addr));

        stack_pos += jump_size;
    }
    printf("DONE -----------------------------\n\n");
}



bool is_expression(const node_type type) {

    switch (type) {
    case INTEGER_CONSTANT_NODE: case UNARY_OP_NODE:
        return true;
    default: return false;
    }
}

bool is_statement(const node_type type) {
    switch (type) {
    case STATEMENT_NODE: case BLOCK_STATEMENT_NODE: case RETURN_STATEMENT_NODE: case IF_STATEMENT_NODE:
        return true;
    default: return false;
    }
}






// integer_constant_node
integer_constant_node make_integer_constant_node(const int value) {
    return (integer_constant_node){INTEGER_CONSTANT_NODE, value}; 
}
integer_constant_node make_integer_constant_node_with_str(const string value) {
    return (integer_constant_node){INTEGER_CONSTANT_NODE, atoin(value)}; 
}

// unary_op_node
unary_op_node make_unary_op_node(const unary_op_type set_op_type, const expr_ptr set_expr) {
    return (unary_op_node){UNARY_OP_NODE, set_op_type, set_expr}; 
}

// return_statement_node
return_statement_node make_return_statement_node(const expr_ptr expr) {
    return (return_statement_node){RETURN_STATEMENT_NODE, expr}; 
}

// block_statement_node
block_statement_node make_block_statement_node(const stmt_ptr_array set_statements) {
    return (block_statement_node){BLOCK_STATEMENT_NODE, set_statements};
}


// identifier_node
identifier_node make_identifier_node(const string set_value) {
    return (identifier_node){IDENTIFIER_NODE, set_value}; }

// c_type_node
c_type_node make_c_type_node(const token_type set_type) {
    return (c_type_node){C_TYPE_NODE, set_type}; 
}

// type_node
type_node make_type_node_c_type_token(const token_type set_type) {
    return (type_node){TYPE_NODE, .is_c_type=true, {make_c_type_node(set_type)}};
}
type_node make_type_node_c_type_node(const c_type_node set_type) {
    return (type_node){TYPE_NODE, .is_c_type=true, {set_type}};
}

// function_node
function_node make_function_node(const type_node set_return_type, const string set_name, const type_node set_parameter, const block_statement_node set_body) {
    return (function_node){FUNCTION_NODE, set_return_type, set_name, set_parameter, set_body};
}

// program_node
program_node make_program_node(const function_node set_func) {
    return (program_node){PROGRAM_NODE, set_func};
}
