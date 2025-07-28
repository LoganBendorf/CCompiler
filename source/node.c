

#include "node.h"

#include "logger.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>


static size_t prev_stack_push_size;


string_span node_span() {
    static const string arr[] = {
                     {"PROGRAM_NODE", 12}, {"FUNCTION_NODE", 13}, {"IDENTIFIER_NODE", 15}, {"BLOCK_STATEMENT_NODE", 20}, {"STATEMENT_NODE", 14}, {
                        "RETURN_STATEMENT_NODE", 21}, {"INTEGER_CONSTANT_NODE", 21}, {"IF_STATEMENT_NODE", 18},
               {"CUSTOM_TYPE_NODE", 16}, {"C_TYPE_NODE", 11}, {"TYPE_NODE", 9}
                    };
    return (string_span){arr, sizeof(arr) / sizeof(arr[0]) };
}

string get_node_string(const size_t index) {

    const string_span node_strs = node_span();

    if (index >= node_strs.size) {
        FATAL_ERROR_STACK_TRACE("get_node_string(): Out of bounds index"); }

    return node_strs.strs[index];
}




node_stack g_node_stack = {8,0, {0}};

size_t allign(const size_t size) {
    const size_t to_add = size % g_node_stack.allignment;
    return size + to_add;
}

size_t size_of_node(const node_type type) {
    switch (type) {
    case RETURN_STATEMENT_NODE: return allign(sizeof(return_statement_node)); break;
    case INTEGER_CONSTANT_NODE: return allign(sizeof(integer_constant_node)); break;
    case PROGRAM_NODE:          return allign(sizeof(program_node));          break;

    default: FATAL_ERROR_STACK_TRACE("Node (%.*s) does not have size available", (int) get_node_string(type).size, get_node_string(type).data);
    }
}

size_t size_of_node_at_stack_ptr(const node_stack_ptr ptr) {
    return size_of_node((node_type) *ptr.addr);
}

size_t size_of_node_at_ptr(const node_ptr ptr) {
    return size_of_node((node_type) *ptr.addr);
}

node_stack_ptr g_node_stack_top_ptr() {
    const node_stack_ptr addr = {&g_node_stack.stack[g_node_stack.top], false};
    return addr;
}

node_stack_ptr pop_g_node_stack() { // FIXME make like get_top_of_g_node_stack()
    if (g_node_stack.top == 0) {
        const node_stack_ptr ret_val = {0, true};
        return ret_val;
    }

    const node_stack_ptr ptr  = g_node_stack_top_ptr();
    const size_t size = size_of_node_at_stack_ptr(ptr);
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
    if (size + g_node_stack.top >= node_STACK_SIZE) {
        FATAL_ERROR_STACK_TRACE("Stack OOM"); }

    memcpy((void*) g_node_stack_top_ptr().addr,  (void*) ptr.addr, size);
    
    g_node_stack.top += size;

    prev_stack_push_size = size;
}



static char*  to_free_list[100];
static size_t to_free_list_size;

string node_addr_to_str(const uint8_t* addr) {

    const node_type type = *addr;

    const string node_type_str = get_node_string(type);
    
    char* buffer = malloc(128);
    size_t size = 0;
    switch (type) {
    case RETURN_STATEMENT_NODE: { // This is actually the worst thing ever
        const return_statement_node ret_stmt_nd = *((return_statement_node*) addr);
        const string expr_str = node_addr_to_str(ret_stmt_nd.expr.addr);
        size = sprintf(buffer, "return %.*s;", (int) expr_str.size, expr_str.data);
    } break;
    case INTEGER_CONSTANT_NODE: {
        const integer_constant_node int_constant_nd = *((integer_constant_node*) addr);
        size = sprintf(buffer, "%d", int_constant_nd.value);
    } break;
    
    default: FATAL_ERROR("Cannot convert (%.*s) node to string", (int) node_type_str.size, node_type_str.data);
    }

    to_free_list[to_free_list_size] = buffer;
    to_free_list_size++;
    return (string){buffer, size};
}

void print_g_nodes(void) {

    printf("PRINTING NODES -------------------\n");

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
    const int max_num_width = strlen(str) + 1; 



    stack_pos = 0;
    size_t count = 1;
    while (stack_pos < g_node_stack.top) {

        const uint8_t* addr = g_node_stack.stack + stack_pos;
        
        const node_type type = *addr;

        const string node_type_str = get_node_string(type);
        const string str = node_addr_to_str(addr);

        printf("%*zu: Type (%.*s)%*s, Value (%.*s)\n", 
            (int) max_num_width,                         count++, 
            (int) node_type_str.size,                    node_type_str.data, 
            (int) (max_type_width - node_type_str.size), "", 
            (int) str.size,                              str.data
        );

        const size_t jump_size = size_of_node((node_type) *(addr));

        stack_pos += jump_size;
    }
    printf("DONE -----------------------------\n\n");

    for (size_t i = 0; i < to_free_list_size; i++) {
        free(to_free_list[i]); }
}



bool is_expression(const node_type type) {

    switch (type) {
    case INTEGER_CONSTANT_NODE: 
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
    return (integer_constant_node){INTEGER_CONSTANT_NODE, value}; }
integer_constant_node make_integer_constant_node_with_str(const string value) {
    return (integer_constant_node){INTEGER_CONSTANT_NODE, atoin(value)}; }

// return_statement_node
return_statement_node make_return_statement_node(const expr_ptr expr) {
    return (return_statement_node){RETURN_STATEMENT_NODE, expr}; }
