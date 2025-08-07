

#include "asm_node.h"

#include "node.h"
#include "structs.h"
#include "logger.h"
#include "registers.h"

#include <stdint.h>
#include <string.h>



extern bool DEBUG;
extern bool TEST;



static size_t prev_stack_push_size = 0;



static size_t asm_node_addr_to_str(const uint8_t* addr, char* buf, const size_t buf_size, size_t index);
static size_t move_asm_node_to_str(const move_asm_node mv_nd, char* buf, const size_t buf_size, size_t index);
// static string type_asm_node_to_str(const type_asm_node nd);


#define BUFFER_SIZE 1024 * 8

bool is_operandanble(const asm_node_type type) {
    switch (type) {
        case INT_IMMEDIATE_ASM_NODE: case REGISTER_ASM_NODE: 
            return true;
        default: 
        return false;
    }
}


[[nodiscard]] string_span asm_node_span() { // TODO static?
    static const string arr[] = {
                {"PROGRAM_ASM_NODE", 16}, {"FUNCTION_ASM_NODE", 17},{"INT_IMMEDIATE_ASM_NODE", 26},
                {"MOVE_ASM_NODE", 13}, {"RETURN_ASM_NODE", 15}, {"BLOCK_ASM_NODE", 14}, {"UNARY_OP_ASM_NODE", 17},
            {"REGISTER_ASM_NODE", 17}, {"STACK_LOCATION_ASM_NODE", 20}, {"IDENTIFIER_ASM_NODE", 19},
            {"TEMP_REG_ASM_NODE", 17}
    };
    return (string_span){arr, sizeof(arr) / sizeof(arr[0]) };
}

string get_asm_node_string(const size_t index) {

    const string_span asm_node_strs = asm_node_span();

    if (index >= asm_node_strs.size) {
        FATAL_ERROR_STACK_TRACE("get_asm_node_string(): Out of bounds index, (%zu)", index); }

    return asm_node_strs.strs[index];
}


// TODO Clear in main.
size_t elem_size[ASM_NODE_STACK_SIZE];
int elem_size_index = 0;
asm_node_stack g_asm_node_stack = {8,0, {0}};

[[nodiscard]] static size_t allign(const size_t size) {
    const size_t to_add = size % g_asm_node_stack.allignment;
    return size + to_add;
}

[[nodiscard]] static size_t size_of_asm_node(const asm_node_type type) {
    switch (type) {
    case FUNCTION_ASM_NODE:      return allign(sizeof(function_asm_node));      break;
    case INT_IMMEDIATE_ASM_NODE: return allign(sizeof(int_immediate_asm_node)); break;
    case RETURN_ASM_NODE:        return allign(sizeof(return_asm_node));        break;
    case PROGRAM_ASM_NODE:       return allign(sizeof(program_asm_node));       break;
    case REGISTER_ASM_NODE:      return allign(sizeof(register_asm_node));      break;
    case UNARY_OP_ASM_NODE:      return allign(sizeof(unary_op_asm_node));      break;
    case BLOCK_ASM_NODE:         return allign(sizeof(block_asm_node));         break;
    case (TEMP_REG_ASM_NODE):    return allign(sizeof(temp_reg_asm_node));      break;

    default: 
        FATAL_ERROR_STACK_TRACE("Asm Node (%.*s) does not have size available", (int) get_asm_node_string(type).size, get_asm_node_string(type).data);
        return 0;
    }
}

[[nodiscard]] static size_t size_of_asm_node_at_ptr(const asm_node_ptr ptr) {
    return size_of_asm_node((asm_node_type) *ptr.addr);
}

[[nodiscard]] static asm_node_stack_ptr g_asm_node_stack_top_ptr() {
    const asm_node_stack_ptr addr = {&g_asm_node_stack.stack[g_asm_node_stack.top], false};
    return addr;
}

static size_t top_traversal_index;
void   set_top_traversal_index(const size_t val) { top_traversal_index = val; }
size_t get_g_asm_node_stack_top_value() { return g_asm_node_stack.top; }
int    get_elem_size_index() { return elem_size_index; }

static int elem_size_traversal_index;
void set_elem_size_traversal_index(const int val) {elem_size_traversal_index = val; }



asm_node_stack_ptr pop_g_asm_node_stack() {
    if (top_traversal_index == 0) {
        const asm_node_stack_ptr ret_val = {0, true};
        return ret_val;
    }

    if (elem_size_traversal_index == -1) {
        FATAL_ERROR_STACK_TRACE("asm_node_stack underflow"); }

    const size_t size = elem_size[elem_size_traversal_index--];
    if (size > top_traversal_index) {
        FATAL_ERROR_STACK_TRACE("asm_node size to pop is greater than asm_node stack top. Would lead to underflow, bad!!!"); }

    top_traversal_index -= size;

    const asm_node_stack_ptr addr = {&g_asm_node_stack.stack[top_traversal_index], false};
    return addr;
}


asm_node_stack_ptr get_top_of_g_asm_node_stack() {
    if (g_asm_node_stack.top == 0) {
        const asm_node_stack_ptr ret_val = {0, true};
        return ret_val;
    }

    const size_t size = prev_stack_push_size;
    if (size > g_asm_node_stack.top) {
        const uint8_t* top_addr = g_asm_node_stack_top_ptr().addr;
        const asm_node_type top_type = *top_addr;
        
        FATAL_ERROR_STACK_TRACE("Can not get top if global asm_node stack, would lead to underflow.\nTop = %zu, request size = %zu. Top type = (%.*s)", 
                                    g_asm_node_stack.top, size,
                                    (int) get_asm_node_string(top_type).size, get_asm_node_string(top_type).data); 
    }

    asm_node_stack_ptr ret = g_asm_node_stack_top_ptr();
    ret.addr -= size;

    return ret;
}

void push_g_asm_node_stack(const asm_node_ptr ptr) {
    // if (ptr.is_none) {
    //     FATAL_ERROR_STACK_TRACE("Can't push none pointer"); }

    const size_t size = size_of_asm_node_at_ptr(ptr);
    if (size + g_asm_node_stack.top >= ASM_NODE_STACK_SIZE) {
        FATAL_ERROR_STACK_TRACE("Stack OOM"); }

    memcpy((void*) g_asm_node_stack_top_ptr().addr,  (void*) ptr.addr, size);
    
    g_asm_node_stack.top += size;

    prev_stack_push_size = size;

    elem_size[elem_size_index] = size;
    elem_size_index++;
}




static size_t function_asm_node_to_str(const function_asm_node func_asm_nd, char* buf, const size_t buf_size, size_t index) {

    const string name = func_asm_nd.name;

    index += (size_t)snprintf(buf + index, buf_size - index, "  .globl %.*s\n%.*s:\n", (int) name.size, name.data, (int) name.size, name.data);

    const block_asm_node body = func_asm_nd.nodes;
    for (size_t i = 0; i < body.nodes.size; i++) {
        const asm_node_ptr asm_nd = body.nodes.values[i];

        // index += (size_t)snprintf(buf + index, buf_size - index, "  ");
        index = asm_node_addr_to_str(asm_nd.addr, buf, buf_size, index);
        index += (size_t)snprintf(buf + index, buf_size - index, "\n");
    }

    return index;
}

static size_t move_asm_node_to_str(const move_asm_node mv_nd, char* buf, const size_t buf_size, size_t index) {

    
    index += (size_t)snprintf(buf + index, buf_size - index, "  movl ");

    index = asm_node_addr_to_str((const uint8_t*) mv_nd.src, buf, buf_size, index);

    index += (size_t)snprintf(buf + index, buf_size - index, ", ");
    
    index = asm_node_addr_to_str((const uint8_t*) mv_nd.dst, buf, buf_size, index);

    index += (size_t)snprintf(buf + index, buf_size - index, "\n");

    return index;
}

// Move expression, apply operand to register
static size_t unary_op_asm_node_to_str(unary_op_asm_node un_op_asm_nd, char* buf, const size_t buf_size, size_t index) {

    const unary_op_type     op_type = un_op_asm_nd.op_type;

    const bool               has_prev = un_op_asm_nd.has_prev;
    const asm_expr_ptr       prev    =  un_op_asm_nd.prev;
    const asm_expr_ptr       src     =  un_op_asm_nd.src;
    const register_asm_node* dst_reg =  un_op_asm_nd.dst_reg;

    if (has_prev) {
        index = asm_node_addr_to_str(prev.addr, buf, buf_size, index);
    }

    index += (size_t)snprintf(buf + index, buf_size - index, "  movl ");

    index = asm_node_addr_to_str(src.addr, buf, buf_size, index);
    index += (size_t)snprintf(buf + index, buf_size - index, ", ");

    
    index = asm_node_addr_to_str((const uint8_t*) dst_reg, buf, buf_size, index);
    index += (size_t)snprintf(buf + index, buf_size - index, "\n  ");
    
    switch (op_type) {

    case BITWISE_COMPLEMENT_OP: index += (size_t)snprintf(buf + index, buf_size - index, "not  "); break;
    case NEGATE_OP:             index += (size_t)snprintf(buf + index, buf_size - index, "neg  "); break;

        
    default:  FATAL_ERROR("Cannot convert (%.*s) unary op type to ASM string", (int) get_unary_op_string(op_type).size, get_unary_op_string(op_type).data);
    }

    index = asm_node_addr_to_str((const uint8_t*) dst_reg, buf, buf_size, index);
    index += (size_t)snprintf(buf + index, buf_size - index, "\n");


    return index;
}


// FIXME Use snprintf
// TODO probably much easier to just return string ptr, saves a lot of space too
static size_t asm_node_addr_to_str(const uint8_t* addr, char* buf, const size_t buf_size, size_t index) {

    const asm_node_type type = *addr;

    const string asm_node_type_str = get_asm_node_string(type);
    
    switch (type) {
    case INT_IMMEDIATE_ASM_NODE: {
        const int_immediate_asm_node int_imm_nd = *((int_immediate_asm_node*) addr);
        index += (size_t)snprintf(buf + index, buf_size - index, "$%d", int_imm_nd.value);
    } break;
    case RETURN_ASM_NODE: {
        const return_asm_node ret_asm_nd = *((return_asm_node*) addr);

        const asm_expr_ptr expr = ret_asm_nd.expr;
        index = asm_node_addr_to_str(expr.addr, buf, buf_size, index);

        const move_asm_node   mv_nd      = ret_asm_nd.move_nd;
        index = move_asm_node_to_str(mv_nd, buf, buf_size, index);

        index += (size_t)snprintf(buf + index, buf_size - index, "  ret");
    } break;
    case FUNCTION_ASM_NODE: {
        const function_asm_node func_asm_nd = *((function_asm_node*) addr);
        index = function_asm_node_to_str(func_asm_nd, buf, buf_size, index);
    } break;
    case PROGRAM_ASM_NODE: {
        const program_asm_node  prog_asm_nd = *((program_asm_node*) addr);
        const function_asm_node func_asm_nd = prog_asm_nd.main_function;
        index = function_asm_node_to_str(func_asm_nd, buf, buf_size, index);

        index += (size_t)snprintf(buf + index, buf_size - index,".section .note.GNU-stack,\"\",@progbits\n");
    } break;    
    case REGISTER_ASM_NODE: {
        const register_asm_node reg_asm_nd = *((register_asm_node*) addr);
        if (reg_asm_nd.is_temp) {
            index += (size_t)snprintf(buf + index, buf_size - index,"%.*s", (int) reg_asm_nd.temp_reg.name.size, reg_asm_nd.temp_reg.name.data);
        } else if (reg_asm_nd.is_stack) {
            const stack_loc_asm_node stack_loc = reg_asm_nd.stack_loc;
            index += (size_t)snprintf(buf + index, buf_size - index,"%d(%%rbp)", stack_loc.position);
        } else { // Is actually a register
            const string reg_str = get_reg_string(reg_asm_nd.reg);
            index += (size_t)snprintf(buf + index, buf_size - index,"%.*s", (int) reg_str.size, reg_str.data);
        }
    } break;
    case UNARY_OP_ASM_NODE: {
        const unary_op_asm_node un_op_asm_nd = *((unary_op_asm_node*) addr);
        index = unary_op_asm_node_to_str(un_op_asm_nd, buf, buf_size, index);
    } break;
    case TEMP_REG_ASM_NODE: {
        const temp_reg_asm_node temp_reg_asm_nd = *((temp_reg_asm_node*) addr);
        index += (size_t)snprintf(buf + index, buf_size - index,"%.*s", (int) temp_reg_asm_nd.name.size, temp_reg_asm_nd.name.data);

    } break;


    default: 
        FATAL_ERROR("Cannot convert (%.*s) asm_node to ASM string", (int) asm_node_type_str.size, asm_node_type_str.data);
        return 0;
    }

    return index;
}

void print_asm_program(const program_asm_node prog, const string msg) {

    static const char* padding = "---------------";
    const size_t default_pad_size = 13;
    int dash_size = (int) default_pad_size - (int) msg.size;
    if (dash_size < 0) {
        dash_size = 0; }

    printf("PRINTING ASM PROGRAM %.*s%.*s\n", (int) msg.size, msg.data, dash_size, padding);
    static char buf[BUFFER_SIZE];
    const size_t start_index = 0;
    const size_t end_index = asm_node_addr_to_str((const uint8_t*) &prog, buf, BUFFER_SIZE, start_index);
    printf("%.*s", (int) end_index + 1, buf + start_index);
    printf("DONE -----------------------------\n\n");
}

string get_assembly(const program_asm_node prog) {
    static char buf[BUFFER_SIZE];
    const size_t start_index = 0;
    const size_t end_index = asm_node_addr_to_str((const uint8_t*) &prog, buf, BUFFER_SIZE, start_index);
    
    if (end_index == BUFFER_SIZE) {
        buf[BUFFER_SIZE - 1] = 0;
        return (string){buf, BUFFER_SIZE - 1};
    } else {
        buf[end_index + 1] = 0; 
        return (string){buf, end_index + 1};
    }
}

void print_g_asm_nodes(const string msg) {

    static const char* padding = "---------------";
    const size_t default_pad_size = 15;
    int dash_size = (int) default_pad_size - (int) msg.size;
    if (dash_size < 0) {
        dash_size = 0; }

    printf("PRINTING ASM NODES %.*s%.*s\n", (int) msg.size, msg.data, dash_size, padding);

    size_t max_type_width = 0;
    size_t stack_pos = 0;
    size_t num_asm_nodes = 0;
    while (stack_pos < g_asm_node_stack.top) {

        
        const uint8_t* addr = g_asm_node_stack.stack + stack_pos;
        
        const asm_node_type type = *addr;
        
        const string asm_node_type_str = get_asm_node_string(type);
        max_type_width = asm_node_type_str.size > max_type_width ? asm_node_type_str.size : max_type_width;
        
        num_asm_nodes++;

        const size_t jump_size = size_of_asm_node((asm_node_type) *(addr));

        stack_pos += jump_size;
    }

    char str[32];
    sprintf(str, "%zu", num_asm_nodes);
    const size_t max_num_width = strlen(str) + 1; 



    stack_pos = 0;
    size_t count = 1;
    while (stack_pos < g_asm_node_stack.top) {

        const uint8_t* addr = g_asm_node_stack.stack + stack_pos;
        
        const asm_node_type type = *addr;

        const string asm_node_type_str = get_asm_node_string(type);

        static char buf[BUFFER_SIZE];
        const size_t start_index = 0;
        const size_t end_index = asm_node_addr_to_str(addr, buf, BUFFER_SIZE, start_index);

        bool found_newline = false;
        for (size_t i = 0; i < end_index; i++) {
            if (buf[i] == '\n') { found_newline = true; }
        }

        const char* add_newline = found_newline ? "\n" : "";

        const int pad_size = found_newline ? 0 : (int) max_type_width - (int) asm_node_type_str.size;

        printf("%*zu: Type (%.*s)%s%*s'%.*s'\n", 
            (int) max_num_width,          count++, 
            (int) asm_node_type_str.size, asm_node_type_str.data, 
                  add_newline,
                  pad_size,               "", 
            (int) end_index + 1,          buf + start_index
        );

        const size_t jump_size = size_of_asm_node((asm_node_type) *(addr));

        stack_pos += jump_size;
    }
    printf("DONE -----------------------------\n\n");
}



// Used so temp registers can be transformed later
[[nodiscard]] static asm_node_stack_ptr place_on_stack(const uint8_t* asm_node_addr) {
    push_g_asm_node_stack((asm_node_ptr){asm_node_addr});

    const asm_node_stack_ptr stack_ptr = get_top_of_g_asm_node_stack();
    if (stack_ptr.is_none) {
        FATAL_ERROR_STACK_TRACE("Failed to get top of g_asm_node_stack"); }

    return stack_ptr;
}



// int_immediate_asm_node
int_immediate_asm_node make_int_immediate_asm_node(const int set_value) {
    return (int_immediate_asm_node){INT_IMMEDIATE_ASM_NODE, set_value};
}

// temp_reg_asm_node
temp_reg_asm_node make_temp_reg_asm_node(const string set_name) {
    return (temp_reg_asm_node){TEMP_REG_ASM_NODE, set_name};
}

// block_asm_node
block_asm_node make_block_asm_node(const asm_node_ptr_array set_statements) {
    return (block_asm_node){BLOCK_ASM_NODE, set_statements};
}

// function_asm_node
function_asm_node make_function_asm_node(const string set_name, const block_asm_node set_nodes) {
    return (function_asm_node){FUNCTION_ASM_NODE, set_name, set_nodes};
}

// program_asm_node
program_asm_node make_program_asm_node(const function_asm_node set_func) {
    return (program_asm_node){PROGRAM_ASM_NODE, set_func};
}

// stack_loc_asm_node
stack_loc_asm_node make_stack_loc_asm_node(const int set_position) {
    return (stack_loc_asm_node){STACK_LOCATION_ASM_NODE, set_position};
}

// register_asm_node. These are automatically placed on stack temp registers can be transformed later.
register_asm_node* make_register_asm_node(const temp_reg_asm_node set_temp_reg) {
    const register_asm_node reg_asm_nd = (register_asm_node){REGISTER_ASM_NODE, .is_temp=true, .is_stack=false, {.temp_reg=set_temp_reg}};
    return (register_asm_node*) place_on_stack((const uint8_t*) &reg_asm_nd).addr; // Bruh
}

register_asm_node* make_register_asm_node_with_stack_loc(const stack_loc_asm_node stack_loc) {
    const register_asm_node reg_asm_nd = (register_asm_node){REGISTER_ASM_NODE, .is_temp=false, .is_stack=true, {.stack_loc=stack_loc}};
    return (register_asm_node*) place_on_stack((const uint8_t*) &reg_asm_nd).addr; // Bruh
}

register_asm_node make_register_asm_node_with_stack_loc_dont_place_on_stack(const stack_loc_asm_node stack_loc) {
    return(register_asm_node){REGISTER_ASM_NODE, .is_temp=false, .is_stack=true, {.stack_loc=stack_loc}};
}

register_asm_node* make_register_asm_node_with_x86(const x86_register set_reg) {
    const register_asm_node reg_asm_nd = (register_asm_node){REGISTER_ASM_NODE, .is_temp=false, .is_stack=false, {.reg=set_reg}};
    return (register_asm_node*) place_on_stack((const uint8_t*) &reg_asm_nd).addr; // Bruh
}

// unary_op_asm_node
unary_op_asm_node make_unary_op_asm_node(const unary_op_type op_type, const asm_expr_ptr set_src, const register_asm_node* set_dst) {
    return (unary_op_asm_node){UNARY_OP_ASM_NODE, .op_type=op_type, .has_prev=false, .src=set_src, .dst_reg=set_dst};
}
unary_op_asm_node make_unary_op_asm_node_wth_previous(const unary_op_type op_type, const asm_expr_ptr prev, const asm_expr_ptr set_src, const register_asm_node* set_dst) {
    return (unary_op_asm_node){UNARY_OP_ASM_NODE, .op_type=op_type, .has_prev=true, .prev=prev, .src=set_src, .dst_reg=set_dst};
}



// move_asm_node
[[nodiscard]] move_asm_node make_move_asm_node(const register_asm_node* set_src, const register_asm_node* set_dst) {
    return (move_asm_node){MOVE_ASM_NODE, set_src, set_dst};
}

// return_asm_node
[[nodiscard]] return_asm_node make_return_asm_node(const asm_expr_ptr set_expr, const move_asm_node set_move_nd) {
    return (return_asm_node){RETURN_ASM_NODE, set_expr, set_move_nd};
}

// identifier_asm_node
identifier_asm_node make_identifier_asm_node(const string set_name) {
    return (identifier_asm_node){IDENTIFIER_ASM_NODE, set_name};
}
