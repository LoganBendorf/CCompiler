

#include "codegen.h"
#include "asm_node.h"
#include "asm_node_mem.h"
#include "tac_node.h"
#include "tac_node_mem.h"
#include "logger.h"

#include <stddef.h>
#include <stdint.h>


extern bool DEBUG;
extern bool TEST;

extern tac_node_stack g_tac_node_stack;
extern tac_node_stack g_asm_node_stack;


#define ERR_RET_EXPECT_ASM_NODE_PTR(fmt, ...) \
    do { \
    fprintf(stderr, "CODEGEN ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    return (expect_asm_node_ptr){.is_error=true}; \
    } while(0)

#define ERR_RET_EXPECT_ASM_EXPR_PTR_AND_DST(fmt, ...) \
    do { \
    fprintf(stderr, "CODEGEN ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    return (expect_asm_expr_ptr_and_dst){.is_error=true}; \
    } while(0)

#define LOG_ERR(fmt, ...) \
    do { \
    fprintf(stderr, "CODEGEN ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    } while(0)

// Dear god only use if the type does not contain an address already, if it does, USE THAT ADDRESS
#define TAC_NODE_TO_TAC_NODE_PTR(x) \
    (tac_node_ptr){(const uint8_t*) &(x)}

#define GET_ASM_NODE_PRINT_PARAMS(x)    \
    (int) get_asm_node_string(x).size, get_asm_node_string(x).data

#define GET_TAC_NODE_PRINT_PARAMS(x)    \
    (int) get_tac_node_string(x).size, get_tac_node_string(x).data
    


typedef struct {
    const bool is_error;
    union {
        const asm_node_ptr node;
        const string       error_msg;
    };
} expect_asm_node_ptr;

typedef struct {
    const bool is_error;
    union {
        const block_asm_node node;
        const string         error_msg;
    };
} expect_block_asm_node;

typedef struct {
    const bool is_error;
    const bool has_dst;
    union {
        struct {
            const asm_expr_ptr expr;
            const asm_expr_ptr dst;
        };
        const string       error_msg;
    };
} expect_asm_expr_ptr_and_dst;


[[nodiscard]] static expect_asm_node_ptr parse_tac_node(const tac_node_ptr nd);



static asm_node_ptr asm_stmt_pool[1024 * 10];
static size_t       asm_stmt_pool_index;



[[nodiscard]] static expect_asm_node_ptr make_expect_asm_node_ptr(uint8_t* asm_node_addr) {
    push_g_asm_node_stack((asm_node_ptr){asm_node_addr});

    const asm_node_stack_ptr stack_ptr = get_top_of_g_asm_node_stack();
    if (stack_ptr.is_none) {
        ERR_RET_EXPECT_ASM_NODE_PTR("Failed to get top of g_asm_node_stack"); }

    return (expect_asm_node_ptr){false, {{stack_ptr.addr}}};
}

[[nodiscard]] static expect_asm_expr_ptr_and_dst make_expect_asm_expr_ptr_and_dst(uint8_t* asm_node_addr) {
    push_g_asm_node_stack((asm_node_ptr){asm_node_addr});

    const asm_node_stack_ptr stack_ptr = get_top_of_g_asm_node_stack();
    if (stack_ptr.is_none) {
        ERR_RET_EXPECT_ASM_EXPR_PTR_AND_DST("Failed to get top of g_asm_node_stack"); }

    return (expect_asm_expr_ptr_and_dst){.is_error=false, .has_dst=false, {{.expr={stack_ptr.addr}, {0}}}};
}

[[nodiscard]] static expect_asm_expr_ptr_and_dst make_expect_asm_expr_ptr_and_dst_wth_dst(uint8_t* asm_node_addr, asm_expr_ptr dst) {
    push_g_asm_node_stack((asm_node_ptr){asm_node_addr});

    const asm_node_stack_ptr stack_ptr = get_top_of_g_asm_node_stack();
    if (stack_ptr.is_none) {
        ERR_RET_EXPECT_ASM_EXPR_PTR_AND_DST("Failed to get top of g_asm_node_stack"); }

    return (expect_asm_expr_ptr_and_dst){.is_error=false, .has_dst=true, {{.expr={stack_ptr.addr}, dst}}};
}



expect_program_asm_node codegen(const program_tac_node program) {

    // asm_node_pool_index = 0;

    const expect_asm_node_ptr expect_program = parse_tac_node(TAC_NODE_TO_TAC_NODE_PTR(program));
    if (expect_program.is_error) {
        LOG_ERR("Failed to parse tac node tree"); 
        return (expect_program_asm_node){.is_error=true};
    }

    const uint8_t* addr = expect_program.node.addr;

    const asm_node_type type = *(addr);

    if (type != PROGRAM_ASM_NODE) {
        LOG_ERR("Code generation failed to produce Program ASM Node, got(%.*s)", GET_ASM_NODE_PRINT_PARAMS(type)); 
        return (expect_program_asm_node){.is_error=true};
    }

    const program_asm_node* program_ptr = (const program_asm_node*) addr;

    return (expect_program_asm_node){.is_error=false, {*program_ptr}};
}



static expect_block_asm_node parse_tac_block_stmt(const block_tac_node block) {

    const size_t start_index = asm_stmt_pool_index;

    const size_t        size   = block.nodes.size;
    const tac_node_ptr* values = block.nodes.values;

    for (size_t i = 0; i < size; i++) {
        const expect_asm_node_ptr expect = parse_tac_node(values[i]);
        if (expect.is_error) {
            LOG_ERR("Failed to parse tac node for block statement");
            return (expect_block_asm_node){.is_error=true};
        }

        asm_stmt_pool[asm_stmt_pool_index++] = expect.node;
    }

    const size_t end_index = asm_stmt_pool_index;

    const asm_node_ptr_array arr = {asm_stmt_pool + start_index, end_index - start_index};

    const block_asm_node block_asm_nd = make_block_asm_node(arr);

    return (expect_block_asm_node){false, {block_asm_nd}};
}



[[nodiscard]] static register_asm_node* get_return_destination_register() {
    register_asm_node* dst_reg = make_register_asm_node_with_x86(EAX);
    return dst_reg;
}

[[nodiscard]] static expect_asm_expr_ptr_and_dst parse_tac_expr(const tac_expr_ptr nd) {

    const uint8_t* addr = nd.addr;

    const tac_node_type type = *(addr);

    switch (type) {

    case UNARY_OP_TAC_NODE: {
        const unary_op_tac_node un_op_tac_nd = *((const unary_op_tac_node*) addr);

        const unary_op_type op_type  = un_op_tac_nd.op_type;
        
        const bool has_prev = un_op_tac_nd.has_prev;
        asm_expr_ptr prev={0};
        if (has_prev) {
            const expect_asm_expr_ptr_and_dst expect_prev_expr = parse_tac_expr(un_op_tac_nd.prev);
            if (expect_prev_expr.is_error) {
                ERR_RET_EXPECT_ASM_EXPR_PTR_AND_DST("Failed to generate code for previous unary op expression"); }
            
            prev = expect_prev_expr.expr;
        }


        const tac_expr_ptr tac_src = un_op_tac_nd.src;
        const var_tac_node tac_dst = un_op_tac_nd.dst;

        const expect_asm_expr_ptr_and_dst expect_expr = parse_tac_expr(tac_src);
        if (expect_expr.is_error) {
            ERR_RET_EXPECT_ASM_EXPR_PTR_AND_DST("Failed to generate code for unary op source"); }

        const expect_asm_node_ptr expect_dst = parse_tac_node(TAC_NODE_TO_TAC_NODE_PTR(tac_dst));
        if (expect_dst.is_error) {
            ERR_RET_EXPECT_ASM_EXPR_PTR_AND_DST("Failed to generate code for unary op destination"); }
        const asm_node_type dst_type = (*expect_dst.node.addr);
        if (dst_type != REGISTER_ASM_NODE) {
            ERR_RET_EXPECT_ASM_EXPR_PTR_AND_DST("Unary op destination failed to become register"); }


        const asm_expr_ptr       src      = expect_expr.expr;
              register_asm_node* dst_addr = (register_asm_node*) expect_dst.node.addr;

        if (!has_prev) {
            const unary_op_asm_node un_op_asm_nd = make_unary_op_asm_node(op_type, src, dst_addr);    

            const expect_asm_expr_ptr_and_dst expect = make_expect_asm_expr_ptr_and_dst_wth_dst((uint8_t*) &un_op_asm_nd, (asm_expr_ptr){expect_dst.node.addr});
            return expect;
        } else {
            const unary_op_asm_node un_op_asm_nd = make_unary_op_asm_node_wth_previous(op_type, prev, src, dst_addr);

            const expect_asm_expr_ptr_and_dst expect = make_expect_asm_expr_ptr_and_dst_wth_dst((uint8_t*) &un_op_asm_nd, (asm_expr_ptr){expect_dst.node.addr});
            return expect;
        }
    } break;

    case INT_CONSTANT_TAC_NODE: {
        const int_constant_tac_node int_const_tac_nd = *((const int_constant_tac_node*) addr);

        const int_immediate_asm_node int_immm_asm_nd =  make_int_immediate_asm_node(int_const_tac_nd.value);

        const expect_asm_expr_ptr_and_dst expect = make_expect_asm_expr_ptr_and_dst((uint8_t*) &int_immm_asm_nd);

        return expect;
    } break;

    default: 
        ERR_RET_EXPECT_ASM_EXPR_PTR_AND_DST("Don't know how to parse tac expr (%.*s)", GET_TAC_NODE_PRINT_PARAMS(type));
    }
}

static expect_asm_node_ptr parse_tac_node(const tac_node_ptr nd) {

    const uint8_t* addr = nd.addr;

    const tac_node_type type = *(addr);

    switch (type) {
    
    case PROGRAM_TAC_NODE: {
        const program_tac_node program_tac_nd = *((const program_tac_node*) addr);

        const expect_asm_node_ptr expect_func = parse_tac_node(TAC_NODE_TO_TAC_NODE_PTR(program_tac_nd.main_function));
        if (expect_func.is_error) {
            ERR_RET_EXPECT_ASM_NODE_PTR("Program tac main function failed to become asm") ; }

        const asm_node_type expect_func_type = *(expect_func.node.addr);
        if (expect_func_type != FUNCTION_ASM_NODE) {
            ERR_RET_EXPECT_ASM_NODE_PTR("Program tac main function failed to become asm functions") ; }

        const function_asm_node asm_func = *((const function_asm_node*) expect_func.node.addr);

        const program_asm_node asm_program = make_program_asm_node(asm_func);

        const expect_asm_node_ptr expect = make_expect_asm_node_ptr((uint8_t*) &asm_program);

        return expect;
    } break;

    case FUNCTION_TAC_NODE: {
        const function_tac_node func_tac_nd = *((const function_tac_node*) addr);
        const string         name  = func_tac_nd.name;
        const block_tac_node nodes = func_tac_nd.nodes;

        const expect_block_asm_node expect_block = parse_tac_block_stmt(nodes);
        if (expect_block.is_error) {
            ERR_RET_EXPECT_ASM_NODE_PTR("Function body failed to become asm"); }

        const block_asm_node block = expect_block.node;

        const function_asm_node fund_asm_nd = make_function_asm_node(name, block);

        const expect_asm_node_ptr expect = make_expect_asm_node_ptr((uint8_t*) &fund_asm_nd);

        return expect;
    } break;

    case RETURN_TAC_NODE: {
        const return_tac_node ret_stmt_nd = *((const return_tac_node*) addr);

        const expect_asm_expr_ptr_and_dst expect_expr = parse_tac_expr((tac_expr_ptr){ret_stmt_nd.expr.addr});
        if (expect_expr.is_error) {
            ERR_RET_EXPECT_ASM_NODE_PTR("Failed to generate code for return statement expression"); }
        const bool prev_constains_dst = expect_expr.has_dst;

        const expect_asm_node_ptr expect_src = parse_tac_node(TAC_NODE_TO_TAC_NODE_PTR(ret_stmt_nd.src));
        if (expect_src.is_error) {
            ERR_RET_EXPECT_ASM_NODE_PTR("Failed to generate code for return statement source"); }
        
        const asm_node_type src_type = *(expect_src.node.addr);
        if (src_type != REGISTER_ASM_NODE) {
            ERR_RET_EXPECT_ASM_NODE_PTR("Return statement source failed to become register"); }

        const asm_expr_ptr expr = expect_expr.expr;

        register_asm_node* src_addr = ((register_asm_node*) expect_src.node.addr);

        register_asm_node* dst_addr = get_return_destination_register();

        if (prev_constains_dst) {
            const move_asm_node move_asm_nd = make_move_asm_node(src_addr, dst_addr);
    
            const return_asm_node ret_asm_nd = make_return_asm_node(expr, move_asm_nd);
    
            const expect_asm_node_ptr expect = make_expect_asm_node_ptr((uint8_t*) &ret_asm_nd);
            return expect;
        } else {
            const return_asm_node ret_asm_nd = make_return_asm_node_without_move(expr);
    
            const expect_asm_node_ptr expect = make_expect_asm_node_ptr((uint8_t*) &ret_asm_nd);
            return expect;
        }

    } break;

    case VAR_TAC_NODE: {
        const var_tac_node var_tac_nd = *((const var_tac_node*) addr);

        const temp_reg_asm_node temp_reg_asm_nd = make_temp_reg_asm_node(var_tac_nd.name);
        
        const register_asm_node* reg_asm_nd_addr = make_register_asm_node(temp_reg_asm_nd);

        const expect_asm_node_ptr expect = make_expect_asm_node_ptr((uint8_t*) reg_asm_nd_addr);

        return expect;
    } break;

    default: 
        ERR_RET_EXPECT_ASM_NODE_PTR("Don't know how to parse tac node (%.*s)", GET_TAC_NODE_PRINT_PARAMS(type));
    }
}















