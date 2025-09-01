
#include "parser.h"

#include "node.h"
#include "node_mem.h"
#include "logger.h"
#include "token.h"
#include "macros.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


extern bool DEBUG;
extern bool TEST;

extern token_stack g_token_stack;
extern node_stack  g_node_stack;


static size_t tok_pos;
static token  prev_tok;

static stmt_ptr stmt_pool[1024 * 10];
static size_t   stmt_pool_index;


typedef enum {
    LOWEST = 0, PREFIX = 2, HIGHEST = 255
} precedences;  

typedef struct {
    const bool is_error;
    union {
        const expr_ptr expression;
        const string   error_msg;
    };
} expect_expr_ptr;

typedef struct {
    const bool is_error;
    union {
        const stmt_ptr statement;
        const string   error_msg;
    };
} expect_stmt_ptr;

typedef struct {
    const bool is_error;
    union {
        const block_statement_node block_stmt;
        const string   error_msg;
    };
} expect_block_stmt;


#define LOG_ERR(fmt, ...) \
    do { \
    fprintf(stderr, "PARSER ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    } while(0)

#define ERR_RET_EXPECT_EXPR_PTR(fmt, ...) \
    do { \
    fprintf(stderr, "PARSER ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    return (expect_expr_ptr){.is_error=true}; \
    } while(0)

#define ERR_RET_EXPECT_STMT_PTR(fmt, ...) \
    do { \
    fprintf(stderr, "PARSER ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    return (expect_stmt_ptr){.is_error=true}; \
    } while(0)

#define ERR_RET_EXPECT_BLOCK_STMT(fmt, ...) \
    do { \
    fprintf(stderr, "PARSER ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    return (expect_block_stmt){.is_error=true}; \
    } while(0)

#define push_err_ret_expect_expr_ptr(fmt, ...) \
    do { \
        const string cur_tok_str = get_token_string(peek().type); \
        ERR_RET_EXPECT_EXPR_PTR(fmt ". Current token (%.*s)" \
                   __VA_OPT__(,) __VA_ARGS__, \
                   (int)cur_tok_str.size, cur_tok_str.data); \
        return (expect_expr_ptr){}; \
    } while (0)

#define advance_and_check_ret_expect_expr_ptr(fmt, ...)                 \
    do {                                                                \
    if (likely(tok_pos < g_token_stack.top)) {                          \
        memcpy(&prev_tok, &g_token_stack.stack[tok_pos], sizeof(token));\
        tok_pos++;                                                      \
    }                                                                   \
    if (unlikely(tok_pos >= g_token_stack.top)) {                       \
        push_err_ret_expect_expr_ptr(fmt __VA_OPT__(,) __VA_ARGS__);    \
    }                                                                   \
    } while (0)

#define push_err_ret_expect_stmt_ptr(fmt, ...) \
    do { \
        const string cur_tok_str = get_token_string(peek().type); \
        ERR_RET_EXPECT_STMT_PTR(fmt ". Current token (%.*s)" \
                   __VA_OPT__(,) __VA_ARGS__, \
                   (int)cur_tok_str.size, cur_tok_str.data); \
        return (expect_stmt_ptr){}; \
    } while (0)

#define advance_and_check_ret_expect_stmt_ptr(fmt, ...)                 \
    do {                                                                \
    if (likely(tok_pos < g_token_stack.top)) {                          \
        memcpy(&prev_tok, &g_token_stack.stack[tok_pos], sizeof(token));\
        tok_pos++;                                                      \
    }                                                                   \
    if (unlikely(tok_pos >= g_token_stack.top)) {                       \
        push_err_ret_expect_stmt_ptr(fmt __VA_OPT__(,) __VA_ARGS__);    \
    }                                                                   \
    } while (0)

// Input is type
#define TOKEN_PRINT_PARAMS(x) \
    (int) get_token_string(x).size, get_token_string(x).data

#define EXPR_PTR_PRINT_PARAMS(x) \
    (int) get_node_string(*x.addr).size, get_node_string(*x.addr).data




static expect_expr_ptr   parse_expression       (const size_t precedence, const token tok);
static expect_expr_ptr   parse_prefix_expression(const token tok);
static expect_stmt_ptr   parse_statement        (const token tok);
static expect_block_stmt parse_block_statement();

static size_t numeric_precedence(const token_type type);
static token  peek();
static token_type peek_type();

static bool is_expression_keyord(const token_type type);
static bool is_statement_keyord(const token_type type);
static bool is_c_type(const token_type type);
static bool is_identifier(const token_type type);


static bool is_identifier(const token_type type) {
    switch (type) {
    case IDENTIFIER: case MAIN:
        return true;
    default: return false;
    }
}

static bool is_expression_keyord(const token_type type) {
    switch (type) {
    case INT_LIT: case OPEN_PAREN: case LINE_END: case IDENTIFIER:
        return true;
    default: return false;
    }
}

static bool is_statement_keyord(const token_type type) {

    if (is_c_type(type)) {
        return true; }

    switch (type) {
    case RETURN: case IF: case LINE_END: case C_TYPE:
        return true;
    default: return false;
    }
}

static bool is_c_type(const token_type type) {
    switch (type) {
    case VOID: case INT:
        return true;
    default:
        return false;
    }
}

expect_program_node parse() {

    tok_pos = 0;
    stmt_pool_index = 0;
    
    const token set_prev_tok = (token){LINE_END, {}, 0, 0};
    memcpy(&prev_tok, &set_prev_tok, sizeof(token));

    stmt_ptr program_start;

    while (peek_type() != LINE_END) {

        if (is_expression_keyord(peek_type())) {
            LOG_ERR("Unexpected expression (%.*s)", (int) peek().data.size, peek().data.data); 
            return (expect_program_node){.is_error=true};
            // parse_expression(LOWEST, peek());
        } else if (is_statement_keyord(peek_type())) {
            const expect_stmt_ptr stmt = parse_statement(peek());
            if (stmt.is_error) {
                // LOG_ERR("BRUH"); 
                return (expect_program_node){.is_error=true};
            }

            const uint8_t* addr = stmt.statement.addr;
            
            const node_type type = *(addr);

            if (type != FUNCTION_NODE) {
                LOG_ERR("parse() did not end with a Function Node, got(%.*s)", (int) get_node_string(type).size, get_node_string(type).data); 
                return (expect_program_node){.is_error=true};
            }

            program_start.addr = addr;

        } else {
            LOG_ERR("Don't know how to parse (%.*s)", (int) get_token_string(peek_type()).size, get_token_string(peek_type()).data);
            return (expect_program_node){.is_error=true};
        }
    }

    const function_node* func_ptr = (const function_node*) program_start.addr;

    if (tok_pos != g_token_stack.top) {
        LOG_ERR("Extra junk at end of parsing (%.*s)", (int) get_token_string(peek_type()).size, get_token_string(peek_type()).data);
        return (expect_program_node){.is_error=true};
    }

    return (expect_program_node){.is_error=false, {make_program_node(*func_ptr)}};

}

static token peek() {
    if (tok_pos >= g_token_stack.top) {
        return (token){LINE_END, {}, 0, 0}; }
    return g_token_stack.stack[tok_pos];
}

static token_type peek_type() {
    return peek().type;
}

static expect_expr_ptr make_expect_expr_ptr(const uint8_t* node_addr) {
    push_g_node_stack((node_ptr){node_addr, false});

    const node_stack_ptr stack_ptr = get_top_of_g_node_stack();
    if (stack_ptr.is_none) {
        ERR_RET_EXPECT_EXPR_PTR("bruh"); }


    return (expect_expr_ptr){false, {{stack_ptr.addr}}};
}

static expect_stmt_ptr make_expect_stmt_ptr(const uint8_t* node_addr) {
    push_g_node_stack((node_ptr){node_addr, false});

    const node_stack_ptr stack_ptr = get_top_of_g_node_stack();
    if (stack_ptr.is_none) {
        ERR_RET_EXPECT_STMT_PTR("bruh"); }

    return (expect_stmt_ptr){false, {{stack_ptr.addr}}};
}

static size_t binary_op_precedence(const binary_op_type type) {
    switch (type) {
        case ADD_OP:            return 11;
        case SUB_OP:            return 11;
        case MUL_OP:            return 12;
        case DIV_OP:            return 12;
        case MOD_OP:            return 12;
        default: return LOWEST;
    }
}

// Infix precedence I think
static size_t numeric_precedence(const token_type type) {
    switch (type) {
    case PLUS:          return 11;
    case MINUS:         return 11;
    case ASTERISK:      return 12;
    case FORWARD_SLASH: return 12;
    case PERCENT:       return 12;

    case OPEN_PAREN:    return 15;
    case OPEN_BRACE:    return 15;
    default:
        return LOWEST;
    }
}



static expect_stmt_ptr parse_c_type_stmt(const token tok) {

    const type_node c_type_nd = make_type_node_c_type_token(tok.type); // I guess this can be passed by value sometimes, custom types can't though
 
    advance_and_check_ret_expect_stmt_ptr("No values after (%.*s)", TOKEN_PRINT_PARAMS(tok.type));

    if (!is_identifier(peek_type())) { // Just something like void
        const expect_stmt_ptr expect = make_expect_stmt_ptr((const uint8_t*) &c_type_nd);
        return expect;
    }

    const expect_expr_ptr ident_expr = parse_expression(HIGHEST, peek());
    if (ident_expr.is_error) {
        push_err_ret_expect_stmt_ptr("Failed to parse C type statement"); }

    const node_type ident_type = *ident_expr.expression.addr;
    
    if (ident_type != IDENTIFIER_NODE) {
        push_err_ret_expect_stmt_ptr("Identifier must follow after C type"); }

    if (peek_type() == OPEN_PAREN) { // Function // TODO Add parse_comma_seperated_list

        advance_and_check_ret_expect_stmt_ptr("No values after open parenthesis in function");

        if (!is_c_type(peek_type())) {
            push_err_ret_expect_stmt_ptr("Function missing parameter type, expected C type, got (%.*s, %.*s)", 
                (int) get_token_string(peek_type()).size, get_token_string(peek_type()).data,
                (int) peek().data.size,                          peek().data.data); 
        }

        const expect_stmt_ptr parameter_stmt = parse_statement(peek());
        if (parameter_stmt.is_error) {
            push_err_ret_expect_stmt_ptr("Failed to parse C type statement"); }
    
        const node_type param_type = *parameter_stmt.statement.addr;
        if (param_type != TYPE_NODE) {
            push_err_ret_expect_stmt_ptr("Function parameter must be type, got (%.*s)", (int) get_node_string(param_type).size, get_node_string(param_type).data); }

        const type_node param_nd = *((const type_node*) parameter_stmt.statement.addr);
        if (!param_nd.is_c_type) {
            push_err_ret_expect_stmt_ptr("Function parameter must be C type"); }

        if (peek_type() != CLOSE_PAREN) {
            push_err_ret_expect_stmt_ptr("Function declaration missing closing parenthesis"); }
        
        advance_and_check_ret_expect_stmt_ptr("No values after close parenthesis in function");

        if (peek_type() != OPEN_BRACE) {
            push_err_ret_expect_stmt_ptr("Function declaration missing open brace"); }

        advance_and_check_ret_expect_stmt_ptr("No values after open brace in function");

        const expect_block_stmt body_stmt_expect = parse_block_statement();
        if (body_stmt_expect.is_error) {
            push_err_ret_expect_stmt_ptr("Failed to parse function body"); }

        if (peek_type() != CLOSE_BRACE) {
            push_err_ret_expect_stmt_ptr("Function declaration missing close brace"); }


        
        tok_pos++;

        const identifier_node ident_nd = *((const identifier_node*) ident_expr.expression.addr);
        const string name = ident_nd.value;

        const block_statement_node body_nd = body_stmt_expect.block_stmt;

        const function_node func_nd = make_function_node(c_type_nd, name, param_nd, body_nd);

        const expect_stmt_ptr expect = make_expect_stmt_ptr((const uint8_t*) &func_nd);

        return expect;
        
    } else {
        push_err_ret_expect_stmt_ptr("Use of '[type] [idenfitier] [%.*s]' is unsupported", (int) get_token_string(peek_type()).size, get_token_string(peek_type()).data);
    }
    
}

static expect_stmt_ptr prefix_parse_statement(const token tok) {

    if (is_c_type(tok.type)) {
        return parse_c_type_stmt(tok);
    }

    switch (tok.type) {
    case RETURN: {
        advance_and_check_ret_expect_stmt_ptr("No values after RETURN");

        const expect_expr_ptr expr = parse_expression(LOWEST, peek());
        if (expr.is_error) {
            push_err_ret_expect_stmt_ptr("Failed to parse return statement expression"); }
        if (peek_type() != SEMICOLON) {
            push_err_ret_expect_stmt_ptr("Return statement missing semicolon"); }

        // tok_pos++;
        advance_and_check_ret_expect_stmt_ptr("No values after SEMICOLON in return statement");

        const return_statement_node return_nd = make_return_statement_node(expr.expression);

        const expect_stmt_ptr expect = make_expect_stmt_ptr((const uint8_t*) &return_nd);

        return expect;
    } break;
    // case IF: { 
    //     auto result = parse_prefix_if();
    //     if (result.has_value()) {
    //         return CAST_UP(object, std::move(*result));
    //     } else {
    //         return CAST_UP(object, std::move(result).error());
    //     }
    // } break;
    case LINE_END:
        ERR_RET_EXPECT_STMT_PTR("Received LINE_END token, either parser error or malformed query");
    default:
        push_err_ret_expect_stmt_ptr("No prefix statement function for (%.*s)", (int) get_token_string(peek_type()).size, get_token_string(peek_type()).data);
    }
}


// static expect_expr_ptr parse_c_type_expr(const token tok) {

//     advance_and_check_ret_expect_expr_ptr("No values after (%.*s)", (int) get_token_string(tok.type).size, get_token_string(tok.type).data);

//     const type_node c_type_nd = make_type_node_c_type_token(tok.type);

//     const expect_expr_ptr expect = make_expect_expr_ptr((uint8_t*) &c_type_nd);

//     return expect;
// }

static expect_expr_ptr parse_prefix_unary_expression(const token tok) {

    advance_and_check_ret_expect_expr_ptr("No values after unary prefix");

    const token_type type = tok.type;

    const expect_expr_ptr expr = parse_expression(HIGHEST, peek());
    if (expr.is_error) {
        push_err_ret_expect_expr_ptr("Error parsing expression for '-'"); }

    unary_op_type op;
    switch (type) {
    case MINUS:              op = NEGATE_OP; break;
    case BITWISE_COMPLEMENT: op = BITWISE_COMPLEMENT_OP; break;
    default: push_err_ret_expect_expr_ptr("Unknown unary operator (%.*s)", (int) tok.data.size, tok.data.data);
    }

    const unary_op_node un_op_nd = make_unary_op_node(op, expr.expression);

    const expect_expr_ptr expect = make_expect_expr_ptr((const uint8_t*) &un_op_nd);

    return expect; 
}

// Don't think this needs a precedence, pretty sure this is the guy who genereates precedence mainly
static expect_expr_ptr parse_prefix_expression(const token tok) {

    if (is_c_type(tok.type)) {
        ERR_RET_EXPECT_EXPR_PTR("C types can not construct expressions"); }


    switch (tok.type) {

    case MINUS: case BITWISE_COMPLEMENT:
        return parse_prefix_unary_expression(tok);

    case OPEN_PAREN: {
        advance_and_check_ret_expect_expr_ptr("No values after open parenthesis");

        const expect_expr_ptr expr = parse_expression(LOWEST, peek());
        if (expr.is_error) {
            push_err_ret_expect_expr_ptr("Failed to parse expression in parenthesis"); }

        if (peek_type() != CLOSE_PAREN) {
            char buf[512];
            const size_t size = node_addr_to_str(expr.expression.addr, buf, 512, 0);

            push_err_ret_expect_expr_ptr("Expected closing parenthesis, got (%.*s). For expression (%.*s)", TOKEN_PRINT_PARAMS(peek_type()), (int) size, buf);
        }

        advance_and_check_ret_expect_expr_ptr("No values after closing parenthesis");

        return expr;
    } break;

    case IDENTIFIER: case MAIN: {
        advance_and_check_ret_expect_expr_ptr("No values after identifier prefix");

        const identifier_node ident_nd = make_identifier_node(tok.data);

        const expect_expr_ptr expect = make_expect_expr_ptr((const uint8_t*) &ident_nd);

        return expect; 
    } break;

    case INT_LIT: { 
        advance_and_check_ret_expect_expr_ptr("No values after integer prefix");

        const integer_constant_node int_const_nd = make_integer_constant_node_with_str(tok.data);

        const expect_expr_ptr expect = make_expect_expr_ptr((const uint8_t*) &int_const_nd);

        return expect; 
    } break;
    // case STR_LIT:
    //     return parse_string_literal(tok);
    case LINE_END:
        ERR_RET_EXPECT_EXPR_PTR("Received LINE_END token, either parser error or malformed query");
    default:
        push_err_ret_expect_expr_ptr("No prefix expression function for (%.*s)", (int) get_token_string(peek_type()).size, get_token_string(peek_type()).data);
    }
}



static expect_expr_ptr parse_binary_infix_expression(const expr_ptr left) {

    const token_type type = peek_type();

    binary_op_type op;
    switch (type) {
    case PLUS:          op = ADD_OP; break;
    case MINUS:         op = SUB_OP; break;
    case ASTERISK:      op = MUL_OP; break;
    case FORWARD_SLASH: op = DIV_OP; break;
    case PERCENT:       op = MOD_OP; break;
    default: 
    push_err_ret_expect_expr_ptr("No infix binary expression for (%.*s)", TOKEN_PRINT_PARAMS(type));
    }

    advance_and_check_ret_expect_expr_ptr("No right hand of infix expression");

    const size_t precedence = binary_op_precedence(op);

    const expect_expr_ptr right = parse_expression(precedence, peek());
    if (right.is_error) {
    push_err_ret_expect_expr_ptr("Failed to parse right hand of expression"); }

    const binary_op_node bin_op_nd = make_binary_op_node(op, left, right.expression);

    const expect_expr_ptr expect = make_expect_expr_ptr((const uint8_t*) &bin_op_nd);
    return expect;
}

static expect_expr_ptr parse_infix_expression(const expr_ptr left) {

    const token_type type = peek_type();

    switch (type) {
    case PLUS: case MINUS: case ASTERISK: case FORWARD_SLASH: case PERCENT:
        return parse_binary_infix_expression(left);
    default:
        push_err_ret_expect_expr_ptr("No infix expression function for (%.*s)", EXPR_PTR_PRINT_PARAMS(left));
    }

}

static expect_expr_ptr parse_expression(const size_t precedence, const token tok) {
    
    // std::cout << "parse_expression called with " << token_type_to_string(peek_type()) << std::endl;

    expect_expr_ptr left = parse_prefix_expression(tok);
    if (left.is_error) {
        return left;
    }

    while (peek_type() != LINE_END &&
           peek_type() != CLOSE_PAREN &&
           peek_type() != SEMICOLON &&
        //    peek_type() != COMMA && 
           precedence < numeric_precedence(peek_type()) ) {
            const expect_expr_ptr prev_left = left; // Having prev_left allows for no-ops
            const expect_expr_ptr temp = parse_infix_expression(left.expression);
            memcpy(&left, &temp, sizeof(expect_expr_ptr));
            if (left.is_error) {
                return left;
            } 

            if (left.expression.addr == prev_left.expression.addr) {
                break; 
            }
        }     
        
        return left;
}



static expect_block_stmt parse_block_statement() {

    const size_t stmt_pool_start_index = stmt_pool_index;

    while (peek_type() != LINE_END && peek_type() != CLOSE_BRACE) {
        if (is_statement_keyord(peek_type())) {
            const expect_stmt_ptr stmt = parse_statement(peek());
            if (stmt.is_error) {
                ERR_RET_EXPECT_BLOCK_STMT("Failed to parse statement in block statement"); }
            
            stmt_pool[stmt_pool_index++] = stmt.statement;
            
        } else {
            ERR_RET_EXPECT_BLOCK_STMT("Non-statement beggining in parse_block_statement (%.*s)", (int) get_token_string(peek_type()).size, get_token_string(peek_type()).data);
        }
    }

    if (peek_type() == LINE_END) {
        ERR_RET_EXPECT_BLOCK_STMT("Block statement failed to terminate, likely missing '}'"); }

    const size_t stmt_pool_end_index = stmt_pool_index;

    const stmt_ptr_array arr = {stmt_pool + stmt_pool_start_index, stmt_pool_end_index - stmt_pool_start_index};

    const block_statement_node block_stmt_nd = make_block_statement_node(arr);

    return (expect_block_stmt){false, {block_stmt_nd}};
}

static expect_stmt_ptr parse_statement(const token tok) {
    
    // std::cout << "parse_expression called with " << token_type_to_string(peek_type()) << std::endl;

    expect_stmt_ptr left = prefix_parse_statement(tok);
    if (left.is_error) {
        return left;
    }
        
    return left;
}





