
#include "asm_node.h"
#include "asm_node_mem.h"

#include "structs.h"
#include "logger.h"
#include "registers.h"
#include "helpers.h"

#include <stdint.h>
#include <string.h>



extern bool DEBUG;
extern bool TEST;

extern asm_node_stack g_asm_node_stack;


#define BUFFER_SIZE 1024 * 8



static size_t asm_node_addr_to_str(const uint8_t* addr, char* buf, const size_t buf_size, size_t index);
static size_t move_asm_node_to_str(const move_asm_node mv_nd, char* buf, const size_t buf_size, size_t index);
// static string type_asm_node_to_str(const type_asm_node nd);



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
            {"TEMP_REG_ASM_NODE", 17}, {"ASM_NODE_LINKED_PTR", 19}
    };
    return (string_span){arr, sizeof(arr) / sizeof(arr[0]) };
}

string get_asm_node_string(const size_t index) {

    const string_span asm_node_strs = asm_node_span();

    if (index >= asm_node_strs.size) {
        FATAL_ERROR_STACK_TRACE("get_asm_node_string(): Out of bounds index, (%zu)", index); }

    return asm_node_strs.strs[index];
}








static int stack_alloc_amt;

static size_t function_asm_node_to_str(const function_asm_node func_asm_nd, char* buf, const size_t buf_size, size_t index) {

    const string name = func_asm_nd.name;

    // Function name and global declaration
    index += (size_t)snprintf(buf + index, buf_size - index, "  .globl %.*s\n%.*s:\n", (int) name.size, name.data, (int) name.size, name.data);

    stack_alloc_amt = func_asm_nd.stack_allocate_ammount;

    // Allocate stack
    if (stack_alloc_amt != 0) {
        index += (size_t)snprintf(buf + index, buf_size - index, "  # Allocate stack\n");
        index += (size_t)snprintf(buf + index, buf_size - index, "  pushq %%rbp\n");
        index += (size_t)snprintf(buf + index, buf_size - index, "  movq %%rsp, %%rbp\n");
        index += (size_t)snprintf(buf + index, buf_size - index, "  subq $%d, %%rsp\n\n", stack_alloc_amt);
    }

    const block_asm_node body = func_asm_nd.nodes;
    for (size_t i = 0; i < body.nodes.size; i++) {
        const asm_node_ptr asm_nd = body.nodes.values[i];

        // index += (size_t)snprintf(buf + index, buf_size - index, "  ");
        index = asm_node_addr_to_str(asm_nd.addr, buf, buf_size, index);
        index += (size_t)snprintf(buf + index, buf_size - index, "\n");
    }

    // Deallocate stack??
    // if (alloc_amt != 0) {
    //     index += (size_t)snprintf(buf + index, buf_size - index, "  subq $%d, %%rsp\n", alloc_amt);
    // }

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

    case BITWISE_COMPLEMENT_OP: index += (size_t)snprintf(buf + index, buf_size - index, "notl "); break;
    case NEGATE_OP:             index += (size_t)snprintf(buf + index, buf_size - index, "negl "); break;

        
    default:  FATAL_ERROR("Cannot convert (%.*s) unary op type to ASM string", (int) get_unary_op_string(op_type).size, get_unary_op_string(op_type).data);
    }

    index = asm_node_addr_to_str((const uint8_t*) dst_reg, buf, buf_size, index);
    index += (size_t)snprintf(buf + index, buf_size - index, "\n");


    return index;
}


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

        if (ret_asm_nd.expr_contains_dst) {
            const asm_expr_ptr expr = ret_asm_nd.expr;
            index = asm_node_addr_to_str(expr.addr, buf, buf_size, index);
    
            const move_asm_node   mv_nd      = ret_asm_nd.move_nd;
            index = move_asm_node_to_str(mv_nd, buf, buf_size, index);
    
            if (stack_alloc_amt != 0) {
                index += (size_t)snprintf(buf + index, buf_size - index, "\n  # Reset stack\n");
                index += (size_t)snprintf(buf + index, buf_size - index, "  movq %%rbp, %%rsp\n");
                index += (size_t)snprintf(buf + index, buf_size - index, "  popq %%rbp\n\n");
            }

            index += (size_t)snprintf(buf + index, buf_size - index, "  ret");

        } else {
            const asm_expr_ptr expr = ret_asm_nd.expr;
            index += (size_t)snprintf(buf + index, buf_size - index, "  movl ");
            index = asm_node_addr_to_str(expr.addr, buf, buf_size, index);
            index += (size_t)snprintf(buf + index, buf_size - index, ", %%eax\n");

            index += (size_t)snprintf(buf + index, buf_size - index, "  ret");
        }

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
    case ASM_NODE_LINKED_PTR: {
        const asm_node_linked_ptr link = *((asm_node_linked_ptr*) addr);
        index = asm_node_addr_to_str(link.prev.addr, buf, buf_size, index);
        index = asm_node_addr_to_str(link.cur.addr, buf, buf_size, index);
    } break;
    case MOVE_ASM_NODE: {
        const move_asm_node mv_asm_nd = *((move_asm_node*) addr);
        const register_asm_node* src = mv_asm_nd.src;
        const register_asm_node* dst = mv_asm_nd.dst;

        index += (size_t)snprintf(buf + index, buf_size - index, "  movl ");
        index = asm_node_addr_to_str((const uint8_t*) src, buf, buf_size, index);
        index += (size_t)snprintf(buf + index, buf_size - index, ", ");
        index = asm_node_addr_to_str((const uint8_t*) dst, buf, buf_size, index);
        index += (size_t)snprintf(buf + index, buf_size - index, "\n");
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
    size_t num_asm_nodes = 0;
    asm_node_stack_iter iter;
    asm_node_stack_iter_constructor(&iter);
    const uint8_t *addr;
    while ((addr = ITER_NEXT(iter)) != NULL) {
    
        const asm_node_type type = *addr;
        
        const string asm_node_type_str = get_asm_node_string(type);
        max_type_width = asm_node_type_str.size > max_type_width ? asm_node_type_str.size : max_type_width;
        
        num_asm_nodes++;
    }

    char str[32];
    sprintf(str, "%zu", num_asm_nodes);
    const size_t max_num_width = strlen(str) + 1; 


    size_t count = 1;
    asm_node_stack_iter_constructor(&iter);
    while ((addr = ITER_NEXT(iter)) != NULL) {
        
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
    }
    printf("DONE -----------------------------\n\n");
}



// Used so temp registers can be transformed later
asm_node_stack_ptr place_on_asm_node_stack(uint8_t* asm_node_addr) {
    push_g_asm_node_stack((asm_node_ptr){asm_node_addr});

    const asm_node_stack_ptr stack_ptr = get_top_of_g_asm_node_stack();
    if (stack_ptr.is_none) {
        FATAL_ERROR_STACK_TRACE("Failed to get top of g_asm_node_stack"); }

    return stack_ptr;
}


// asm_node_linked_ptr
asm_node_linked_ptr make_asm_node_linked_ptr(asm_expr_ptr set_prev, asm_expr_ptr set_cur) {
    return (asm_node_linked_ptr){ASM_NODE_LINKED_PTR, set_prev, set_cur};
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
    return (function_asm_node){FUNCTION_ASM_NODE, set_name, set_nodes, 0};
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
    return (register_asm_node*) place_on_asm_node_stack((uint8_t*) &reg_asm_nd).addr; // Bruh
}

register_asm_node* make_register_asm_node_with_stack_loc(const stack_loc_asm_node stack_loc) {
    const register_asm_node reg_asm_nd = (register_asm_node){REGISTER_ASM_NODE, .is_temp=false, .is_stack=true, {.stack_loc=stack_loc}};
    return (register_asm_node*) place_on_asm_node_stack((uint8_t*) &reg_asm_nd).addr; // Bruh
}

register_asm_node make_register_asm_node_with_stack_loc_dont_place_on_stack(const stack_loc_asm_node stack_loc) {
    return(register_asm_node){REGISTER_ASM_NODE, .is_temp=false, .is_stack=true, {.stack_loc=stack_loc}};
}

register_asm_node* make_register_asm_node_with_x86(const x86_register set_reg) {
    const register_asm_node reg_asm_nd = (register_asm_node){REGISTER_ASM_NODE, .is_temp=false, .is_stack=false, {.reg=set_reg}};
    return (register_asm_node*) place_on_asm_node_stack((uint8_t*) &reg_asm_nd).addr; // Bruh
}

// unary_op_asm_node
unary_op_asm_node make_unary_op_asm_node(const unary_op_type op_type, const asm_expr_ptr set_src, register_asm_node* set_dst) {
    return (unary_op_asm_node){UNARY_OP_ASM_NODE, .op_type=op_type, .has_prev=false, .src=set_src, .dst_reg=set_dst};
}
unary_op_asm_node make_unary_op_asm_node_wth_previous(const unary_op_type op_type, const asm_expr_ptr prev, const asm_expr_ptr set_src, register_asm_node* set_dst) {
    return (unary_op_asm_node){UNARY_OP_ASM_NODE, .op_type=op_type, .has_prev=true, .prev=prev, .src=set_src, .dst_reg=set_dst};
}



// move_asm_node
[[nodiscard]] move_asm_node make_move_asm_node(register_asm_node* set_src, register_asm_node* set_dst) {
    return (move_asm_node){MOVE_ASM_NODE, set_src, set_dst};
}

// return_asm_node
[[nodiscard]] return_asm_node make_return_asm_node(const asm_expr_ptr set_expr, const move_asm_node set_move_nd) {
    return (return_asm_node){RETURN_ASM_NODE, set_expr, true, set_move_nd};
}

[[nodiscard]] return_asm_node make_return_asm_node_without_move(const asm_expr_ptr set_expr) {
    return (return_asm_node){RETURN_ASM_NODE, set_expr, false, {}};
}

// identifier_asm_node
identifier_asm_node make_identifier_asm_node(const string set_name) {
    return (identifier_asm_node){IDENTIFIER_ASM_NODE, set_name};
}
