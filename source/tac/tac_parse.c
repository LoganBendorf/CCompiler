

#include "tac_parse.h"

#include "tac_node.h"
#include "tac_node_mem.h"
#include "logger.h"
#include "node.h"
#include "tac_node.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>


extern bool DEBUG;
extern bool TEST;

extern tac_node_stack g_tac_node_stack;


static size_t temp_vars_allocated;


#define NODE_TO_NODE_PTR(x) \
    (node_ptr){(const uint8_t*) &(x), .is_none=false}

#define ADDR_TO_NODE_PTR(x) \
    (node_ptr){(const uint8_t*) (x), .is_none=false}


#define ERR_RET_EXPECT_TAC_EXPR_PTR_OR_DST(fmt, ...) \
    do { \
    fprintf(stderr, "TAC PARSE ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    return (expect_tac_expr_ptr_and_dst){.is_error=true}; \
    } while(0)


#define ERR_RET_EXPECT_TAC_NODE_PTR(fmt, ...) \
    do { \
    fprintf(stderr, "TAC PARSE ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    return (expect_tac_node_ptr){.is_error=true}; \
    } while(0)

#define ERR_RET_EXPECT_BLOCK_TAC_NODE(fmt, ...) \
    do { \
    fprintf(stderr, "TAC PARSE ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    return (expect_block_tac_node){.is_error=true}; \
    } while(0)


#define LOG_ERR(fmt, ...) \
    do { \
    fprintf(stderr, "TAC PARSE ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    } while(0)

// Helper for printing
#define TAC_PRINT_PARAMS(x)    \
    (int) get_tac_node_string(x).size, get_tac_node_string(x).data

#define NODE_PRINT_PARAMS(x)    \
    (int) get_node_string(x).size, get_node_string(x).data


typedef struct {
    const bool is_error;
    union {
        const tac_node_ptr node;
        const string       error_msg;
    };
} expect_tac_node_ptr;

typedef struct {
    const bool is_error;
    const bool has_dst;
    union {
        struct {
            const tac_expr_ptr expr;
            const tac_expr_ptr dst;
        };
        const string       error_msg;
    };
} expect_tac_expr_ptr_and_dst;

typedef struct {
    const bool is_error;
    union {
        const tac_expr_ptr expr;
        const string       error_msg;
    };
} expect_tac_expr_ptr;

typedef struct {
    const bool is_error;
    union {
        const block_tac_node block_nd;
        const string         error_msg;
    };
} expect_block_tac_node;


static tac_node_ptr tac_stmt_pool[1024 * 10];
static size_t       tac_stmt_pool_index;



[[nodiscard]] static expect_tac_node_ptr        parse_ast_node           (const node_ptr nd);
[[nodiscard]] static expect_block_tac_node      parse_ast_block_stmt     (const block_statement_node block);
[[nodiscard]] static expect_tac_expr_ptr_and_dst parse_ast_expr           (const expr_ptr expr); // Currently only used for return stmt
[[nodiscard]] static expect_tac_node_ptr        make_expect_tac_node_ptr (const uint8_t* tac_node_addr);



static expect_tac_node_ptr make_expect_tac_node_ptr(const uint8_t* tac_node_addr) {
    push_g_tac_node_stack((tac_node_ptr){tac_node_addr});

    const tac_node_stack_ptr stack_ptr = get_top_of_g_tac_node_stack();
    if (stack_ptr.is_none) {
        ERR_RET_EXPECT_TAC_NODE_PTR("Failed to get top of g_tac_node_stack"); }

    return (expect_tac_node_ptr){false, {{stack_ptr.addr}}};
}



static expect_tac_expr_ptr_and_dst make_expect_tac_expr_ptr_and_dst(const uint8_t* tac_expr_addr, bool has_dst, const uint8_t* dist_addr) {
    push_g_tac_node_stack((tac_node_ptr){tac_expr_addr});

    const tac_node_stack_ptr expr_stack_ptr = get_top_of_g_tac_node_stack();
    if (expr_stack_ptr.is_none) {
        ERR_RET_EXPECT_TAC_EXPR_PTR_OR_DST("Failed to get top of g_tac_node_stack"); }

    tac_node_stack_ptr dst_stack_ptr = {0};
    if (has_dst) {
        push_g_tac_node_stack((tac_node_ptr){dist_addr});

        const tac_node_stack_ptr tmp = get_top_of_g_tac_node_stack();
        if (tmp.is_none) {
            ERR_RET_EXPECT_TAC_EXPR_PTR_OR_DST("Failed to get top of g_tac_node_stack"); }

        memcpy(&dst_stack_ptr, &tmp, sizeof(tac_node_ptr));
    }

    return (expect_tac_expr_ptr_and_dst){false, has_dst, { {{expr_stack_ptr.addr}, {dst_stack_ptr.addr}}}};
}



expect_program_tac_node parse_ast(const program_node program) {

    temp_vars_allocated = 0;
    tac_stmt_pool_index = 0;

    const expect_tac_node_ptr ptr = parse_ast_node(NODE_TO_NODE_PTR(program));
    if (ptr.is_error) {
        LOG_ERR("Failed to create tac node tree"); 
        return (expect_program_tac_node){.is_error=true};
    }

    const uint8_t* addr = ptr.node.addr;

    const tac_node_type type = *(addr);

    if (type != PROGRAM_TAC_NODE) {
        LOG_ERR("TAC generation failed to produce Program Tac Node, got(%.*s)", (int) get_tac_node_string(type).size, get_tac_node_string(type).data); 
        return (expect_program_tac_node){.is_error=true};
    }

    const program_tac_node* program_ptr = (const program_tac_node*) addr;

    return (expect_program_tac_node){.is_error=false, {*program_ptr}};
}



// typedef enum {
//     PTR_TYPE_STACK_COPY,
//     PTR_TYPE_HEAP_ARRAY,
//     PTR_TYPE_GLOBAL_POOL
// } ptr_type;

// typedef struct {
//     ptr_type type;
//     size_t count;
//     union {
//         uint8_t stack_copy[256];       
//         uint8_t* heap_array;           
//         struct { size_t offset; } pool;
//     } data;
// } weird_ptr;

// I could, smile
// typedef struct {
//     const size_t capacity;
//     size_t count;
//     uint8_t* data;
// } block_tac_node_container;

static expect_block_tac_node parse_ast_block_stmt(const block_statement_node block) {

    const size_t tac_node_pool_start_index = tac_stmt_pool_index;

    // No malloc but dyn memory, smile
    // uint8_t stmt_ptrs[block.statements.size];
    // block_tac_node_container container = {block.statements.size, 0, stmt_ptrs};


    const stmt_ptr* stmts = block.statements.values;
    const size_t    size  = block.statements.size;

    for (size_t i = 0; i < size; i++) {
        const expect_tac_node_ptr expect = parse_ast_node(ADDR_TO_NODE_PTR(stmts[i].addr));
        if (expect.is_error) {
            ERR_RET_EXPECT_BLOCK_TAC_NODE("Failed to parse ast node for block statement"); }
        
        tac_stmt_pool[tac_stmt_pool_index++] = expect.node;
    }

    const size_t tac_node_pool_end_index = tac_stmt_pool_index;

    const tac_node_ptr_array arr = {tac_stmt_pool + tac_node_pool_start_index, tac_node_pool_end_index - tac_node_pool_start_index};

    const block_tac_node block_tac_nd = make_block_tac_node(arr);

    return (expect_block_tac_node){false, {block_tac_nd}};
}



// returns tmp.x
var_tac_node make_temporary() {
    static const char* tmps[] = {"tmp.0", "tmp.1", "tmp.2", "tmp.3", "tmp.4", "tmp.5", "tmp.6", "tmp.7", "tmp.8",
                          "tmp.9", "tmp.10", "tmp.11", "tmp.12", "tmp.13", "tmp.14", "tmp.15", "tmp.16",
                         "tmp.17", "tmp.18", "tmp.19", "tmp.20", "tmp.21", "tmp.22", "tmp.23", "tmp.24",
                          "tmp.25", "tmp.26", "tmp.27", "tmp.28", "tmp.29", "tmp.30"
    };

    var_tac_node var_tac_nd = make_var_tac_node((string){tmps[temp_vars_allocated], strlen(tmps[temp_vars_allocated])});
    temp_vars_allocated++;
    return var_tac_nd;   
}

static expect_tac_expr_ptr_and_dst parse_ast_expr(const expr_ptr expr) {

    const uint8_t* addr = expr.addr;

    const node_type type = *(expr.addr);

    switch (type) {

    case UNARY_OP_NODE: { // Returns unary destination
        const unary_op_node un_op_nd = *((const unary_op_node*) addr);

        const unary_op_type op_type = un_op_nd.op_type;

        const expr_ptr ast_expr = un_op_nd.expr;

        const expect_tac_expr_ptr_and_dst expect_src = parse_ast_expr(ast_expr);
        if (expect_src.is_error) {
            const node_type nd_err_type = (node_type) (*ast_expr.addr);
            ERR_RET_EXPECT_TAC_EXPR_PTR_OR_DST("Failed to parse AST expression (%.*s)", (int) get_node_string(nd_err_type).size, get_node_string(nd_err_type).data);
        }
        const tac_node_type src_type = *expect_src.expr.addr;
        if (src_type != INT_CONSTANT_TAC_NODE && src_type != VAR_TAC_NODE && src_type != UNARY_OP_TAC_NODE && src_type != BINARY_OP_TAC_NODE) {
            ERR_RET_EXPECT_TAC_EXPR_PTR_OR_DST("Unary AST expression failed to become constant or variable (%.*s)", TAC_PRINT_PARAMS(src_type)); }

        
        // e.g tmp.0 = ~0
        if (!expect_src.has_dst) {
            const tac_expr_ptr src  = expect_src.expr;
            const var_tac_node dst = make_temporary();

            const unary_op_tac_node un_op_tac_node = make_unary_op_tac_node(op_type, src, dst);

            const expect_tac_expr_ptr_and_dst expect = make_expect_tac_expr_ptr_and_dst((const uint8_t*) &un_op_tac_node, true, (const uint8_t*) &dst);
            return expect;
        } else { // e.g tmp.1 = - tmp.0
            const tac_expr_ptr prev = expect_src.expr;
            
            const tac_expr_ptr src  = expect_src.dst;
            const var_tac_node dst = make_temporary();

            const unary_op_tac_node un_op_tac_node = make_unary_op_tac_node_wth_previous(op_type, prev, src, dst);

            const expect_tac_expr_ptr_and_dst expect = make_expect_tac_expr_ptr_and_dst((const uint8_t*) &un_op_tac_node, true, (const uint8_t*) &dst);
            return expect;
        }
    } break;
    
    case BINARY_OP_NODE: {
        const binary_op_node bin_op_nd = *((const binary_op_node*) addr);

        const binary_op_type op_type = bin_op_nd.op_type;

        const expr_ptr left_expr = bin_op_nd.left;
        const expect_tac_expr_ptr_and_dst expect_left = parse_ast_expr(left_expr);
        if (expect_left.is_error) {
            const node_type nd_err_type = (node_type) (*left_expr.addr);
            ERR_RET_EXPECT_TAC_EXPR_PTR_OR_DST("Failed to parse left side of AST binary op (%.*s)", NODE_PRINT_PARAMS(nd_err_type));
        }

        const tac_node_type left_src_type = *expect_left.expr.addr;
        if (left_src_type != INT_CONSTANT_TAC_NODE && left_src_type != VAR_TAC_NODE && left_src_type != UNARY_OP_TAC_NODE && left_src_type != BINARY_OP_TAC_NODE) {
            ERR_RET_EXPECT_TAC_EXPR_PTR_OR_DST("Binary left AST expression failed to become constant or variable (%.*s)", TAC_PRINT_PARAMS(left_src_type)); }


        const expr_ptr right_expr = bin_op_nd.right;
        const expect_tac_expr_ptr_and_dst expect_right = parse_ast_expr(right_expr);
        if (expect_right.is_error) {
            const node_type nd_err_type = (node_type) (*right_expr.addr);
            ERR_RET_EXPECT_TAC_EXPR_PTR_OR_DST("Failed to parse right side of AST binary op (%.*s)", NODE_PRINT_PARAMS(nd_err_type));
        }

        const tac_node_type right_src_type = *expect_right.expr.addr;
        if (right_src_type != INT_CONSTANT_TAC_NODE && right_src_type != VAR_TAC_NODE && right_src_type != UNARY_OP_TAC_NODE && right_src_type != BINARY_OP_TAC_NODE) {
            ERR_RET_EXPECT_TAC_EXPR_PTR_OR_DST("Binary right AST expression failed to become constant or variable (%.*s)", TAC_PRINT_PARAMS(right_src_type)); }


        const var_tac_node dst = make_temporary();

        
        if (expect_left.has_dst && expect_right.has_dst) {
            const binary_op_tac_node bin_op_tac_nd = make_binary_op_tac_node_has_both_prev(op_type, expect_left.expr, expect_right.expr, expect_left.dst, expect_right.dst, dst);
            const expect_tac_expr_ptr_and_dst expect = make_expect_tac_expr_ptr_and_dst((const uint8_t*) &bin_op_tac_nd, true, (const uint8_t*) &dst);
            return expect;
        } else if (expect_left.has_dst) {
            const binary_op_tac_node bin_op_tac_nd = make_binary_op_tac_node_has_left_prev(op_type, expect_left.expr, expect_left.dst, expect_right.expr, dst);
            const expect_tac_expr_ptr_and_dst expect = make_expect_tac_expr_ptr_and_dst((const uint8_t*) &bin_op_tac_nd, true, (const uint8_t*) &dst);
            return expect;
        } else if (expect_right.has_dst) {
            const binary_op_tac_node bin_op_tac_nd = make_binary_op_tac_node_has_right_prev(op_type, expect_left.expr, expect_right.expr, expect_right.dst, dst);
            const expect_tac_expr_ptr_and_dst expect = make_expect_tac_expr_ptr_and_dst((const uint8_t*) &bin_op_tac_nd, true, (const uint8_t*) &dst);
            return expect;
        } else {
            const binary_op_tac_node bin_op_tac_nd = make_binary_op_tac_node(op_type, expect_left.expr, expect_right.expr, dst);   
            const expect_tac_expr_ptr_and_dst expect = make_expect_tac_expr_ptr_and_dst((const uint8_t*) &bin_op_tac_nd, true, (const uint8_t*) &dst);
            return expect;
        }
    } break;

    case INTEGER_CONSTANT_NODE: {
        const integer_constant_node int_const_nd = *((const integer_constant_node*) addr);
        const int value = int_const_nd.value;

        const int_constant_tac_node int_const_tac_nd = make_int_constant_tac_node(value);

        const expect_tac_expr_ptr_and_dst expect = make_expect_tac_expr_ptr_and_dst((const uint8_t*) &int_const_tac_nd, false, nullptr);
        return expect;
    } break;

    default: 
        ERR_RET_EXPECT_TAC_EXPR_PTR_OR_DST("Don't know how to parse ast node for expression (%.*s)", (int) get_node_string(type).size, get_node_string(type).data);
    }

}

static expect_tac_node_ptr parse_ast_node(const node_ptr nd) {

    const uint8_t* addr = nd.addr;

    const node_type type = *(addr);

    switch (type) {
    
    case PROGRAM_NODE: {
        const program_node program_nd = *((const program_node*) addr);
        const expect_tac_node_ptr expect_func = parse_ast_node(NODE_TO_NODE_PTR(program_nd.main_function));
        if (expect_func.is_error) {
            ERR_RET_EXPECT_TAC_NODE_PTR(""); }

        const uint8_t* expect_func_addr = expect_func.node.addr;
        const tac_node_type expect_func_type = *(expect_func_addr);
        if (expect_func_type != FUNCTION_TAC_NODE) {
            const string type_str = get_tac_node_string(expect_func_type);
            ERR_RET_EXPECT_TAC_NODE_PTR("Expected Function Asm Node, got (%.*s)", (int) type_str.size, type_str.data); 
        }

        const function_tac_node func_tac_nd = *((const function_tac_node*) expect_func.node.addr);

        const program_tac_node program_tac_nd = make_program_tac_node(func_tac_nd);

        const expect_tac_node_ptr expect = make_expect_tac_node_ptr((const uint8_t*) &program_tac_nd);

        return expect;
    } break;

    case FUNCTION_NODE: {
        const function_node func_nd = *((const function_node*) addr);
        [[maybe_unused]] const type_node  return_type = func_nd.return_type;
        [[maybe_unused]] const string     name        = func_nd.name;
        [[maybe_unused]] const type_node  param       = func_nd.parameter;
        const block_statement_node body = func_nd.body;

        const expect_block_tac_node expect_block = parse_ast_block_stmt(body);
        if (expect_block.is_error) {
            ERR_RET_EXPECT_TAC_NODE_PTR(""); }

        const block_tac_node block = expect_block.block_nd;

        const function_tac_node fund_tac_nd = make_function_tac_node(name, block);

        const expect_tac_node_ptr expect = make_expect_tac_node_ptr((const uint8_t*) &fund_tac_nd);

        return expect;
    } break;

    case RETURN_STATEMENT_NODE: {
        const return_statement_node ret_stmt_nd = *((const return_statement_node*) addr);
        const expect_tac_expr_ptr_and_dst expect_expr_and_src = parse_ast_expr(ret_stmt_nd.expr);
        if (expect_expr_and_src.is_error) {
            ERR_RET_EXPECT_TAC_NODE_PTR("Failed parse AST return statement expression"); }

        var_tac_node src;
        bool expr_contains_dst = false;
        if (!expect_expr_and_src.has_dst) {
            const var_tac_node tmp = make_temporary(); 
            memcpy(&src, &tmp, sizeof(var_tac_node));
        } else {
            const tac_node_type src_type = *(expect_expr_and_src.dst.addr);
            if (src_type != VAR_TAC_NODE) {
                ERR_RET_EXPECT_TAC_NODE_PTR("Return statement expression source was not var type"); }

            const var_tac_node tmp = *( (var_tac_node*) expect_expr_and_src.dst.addr );
            memcpy(&src, &tmp, sizeof(var_tac_node));

            expr_contains_dst = true;
        }

        const tac_expr_ptr expr = expect_expr_and_src.expr;

        const return_tac_node ret_tac_nd = make_return_tac_node(expr, expr_contains_dst, src);

        const expect_tac_node_ptr expect = make_expect_tac_node_ptr((const uint8_t*) &ret_tac_nd);

        return expect;

    } break;

    default: 
        ERR_RET_EXPECT_TAC_NODE_PTR("Don't know how to parse ast node for (%.*s)", (int) get_node_string(type).size, get_node_string(type).data);
    }
}
















