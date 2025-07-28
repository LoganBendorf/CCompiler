
#include "parser.h"

#include "node.h"
#include "logger.h"
#include "token.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    ADD_OP, SUB_OP, MUL_OP, DIV_OP, NEGATE_OP,
    EQUALS_OP, NOT_EQUALS_OP, LESS_THAN_OP, LESS_THAN_OR_EQUAL_TO_OP, GREATER_THAN_OP, GREATER_THAN_OR_EQUAL_TO_OP,
    OPEN_PAREN_OP, OPEN_BRACKET_OP, DOT_OP, NULL_OP
} operator_type;

typedef enum {
    LOWEST = 0, PREFIX = 6, HIGHEST = 255
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



#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define push_err_ret_expect_expr_ptr(fmt, ...) \
    do { \
        const string cur_tok_str = get_token_string(peek().type); \
        FATAL_ERROR(fmt ". Current token (%.*s)" \
                   __VA_OPT__(,) __VA_ARGS__, \
                   (int)cur_tok_str.size, cur_tok_str.data); \
        return (expect_expr_ptr){}; \
    } while (0)

#define advance_and_check_ret_expect_expr_ptr(x)        \
    do {                                                \
    if (likely(tok_pos < g_token_stack.top)) {          \
        memcpy(&prev_tok, &g_token_stack.stack[tok_pos], sizeof(token)); \
        tok_pos++;                                      \
    }                                                   \
    if (unlikely(tok_pos >= g_token_stack.top)) {       \
        push_err_ret_expect_expr_ptr(x); }              \
    } while (0)

#define push_err_ret_expect_stmt_ptr(fmt, ...) \
    do { \
        const string cur_tok_str = get_token_string(peek().type); \
        FATAL_ERROR(fmt ". Current token (%.*s)" \
                   __VA_OPT__(,) __VA_ARGS__, \
                   (int)cur_tok_str.size, cur_tok_str.data); \
        return (expect_stmt_ptr){}; \
    } while (0)

#define advance_and_check_ret_expect_stmt_ptr(x)        \
    do {                                                \
    if (likely(tok_pos < g_token_stack.top)) {          \
        memcpy(&prev_tok, &g_token_stack.stack[tok_pos], sizeof(token)); \
        tok_pos++;                                      \
    }                                                   \
    if (unlikely(tok_pos >= g_token_stack.top)) {       \
        push_err_ret_expect_stmt_ptr(x); }              \
    } while (0)



extern token_stack g_token_stack;
extern node_stack  g_node_stack;

static size_t tok_pos;
static token  prev_tok;



static expect_expr_ptr parse_expression       (const size_t precedence, const token tok);
static expect_expr_ptr prefix_parse_expression(const token tok);
static expect_stmt_ptr parse_statement        (const token tok);

static size_t numeric_precedence(const token_type type);
static token  peek();
static token_type peek_type();



static bool is_expression_keyord(const token_type type) {
    switch (type) {
    case INT_LIT: case OPEN_PAREN: case LINE_END:
        return true;
    default: return false;
    }
}

static bool is_statement_keyord(const token_type type) {
    switch (type) {
    case RETURN: case IF: case LINE_END:
        return true;
    default: return false;
    }
}

void parse() {
    tok_pos = 0;
    
    const token set_prev_tok = (token){LINE_END, {}, 0, 0};
    memcpy(&prev_tok, &set_prev_tok, sizeof(token));


    if (is_expression_keyord(peek_type())) {
        parse_expression(LOWEST, peek());
    } else if (is_statement_keyord(peek_type())) {
        parse_statement(peek());
    }
}

static token peek() {
    if (tok_pos >= g_token_stack.top) {
        return (token){LINE_END, {}, 0, 0}; }
    return g_token_stack.stack[tok_pos];
}

static token_type peek_type() {
    return peek().type;
}

static expect_expr_ptr make_expect_expr_ptr(const uint8_t* value) {
    return (expect_expr_ptr){false, {{value}}};
}

static expect_stmt_ptr make_expect_stmt_ptr(const uint8_t* value) {
    return (expect_stmt_ptr){false, {{value}}};
}

[[maybe_unused]] static size_t numeric_operator_precedence(const operator_type type) {
    switch (type) {
        case EQUALS_OP:         return 2; break;
        case NOT_EQUALS_OP:     return 2; break;
        case LESS_THAN_OP:      return 3; break;
        case GREATER_THAN_OP:   return 3; break;
        case ADD_OP:            return 4; break;
        case SUB_OP:            return 4; break;
        case MUL_OP:            return 5; break;
        case DIV_OP:            return 5; break;
        case OPEN_PAREN_OP:     return 7; break;
        case OPEN_BRACKET_OP:   return 8; break;
        default:
            return LOWEST;
    }
}

static size_t numeric_precedence(const token_type type) {
    switch (type) {
        // case AS:            return 1; break; 
        // case WHERE:         return 1; break; 
        // case LEFT:          return 1; break; 
        // case EQUAL:         return 2; break;
        // case NOT_EQUAL:     return 2; break;
        // case LESS_THAN:     return 3; break;
        // case GREATER_THAN:  return 3; break;
        // case PLUS:          return 4; break;
        // case MINUS:         return 4; break;
        // case ASTERISK:      return 5; break;
        // case SLASH:         return 5; break;
        case OPEN_PAREN:    return 7; break;
        case OPEN_BRACE:  return 8; break;
        default:
            return LOWEST;
    }
}



static expect_stmt_ptr prefix_parse_statement(const token tok) {
    switch (tok.type) {
    case RETURN: {
        advance_and_check_ret_expect_stmt_ptr("No values after RETURN");

        const expect_expr_ptr expr = parse_expression(LOWEST, peek());
        if (expr.is_error) {
            push_err_ret_expect_stmt_ptr("Failed to parse return statement expression"); }
        if (peek_type() != SEMICOLON) {
            push_err_ret_expect_stmt_ptr("Return statement missing semicolon"); }

        tok_pos++;
        // advance_and_check_ret_expect_stmt_ptr("No values after SEMICOLON in return statement");

        const return_statement_node return_nd = make_return_statement_node(expr.expression);

        push_g_node_stack((node_ptr){(uint8_t*) &return_nd, false});

        const node_stack_ptr stack_ptr = get_top_of_g_node_stack();
        if (stack_ptr.is_none) {
            FATAL_ERROR("bruh"); }

        const expect_stmt_ptr expect = make_expect_stmt_ptr(stack_ptr.addr);

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
        FATAL_ERROR_STACK_TRACE("Received LINE_END token, either parser error or malformed query");
    default:
        push_err_ret_expect_stmt_ptr("No prefix function for (%.*s)", (int) get_token_string(peek_type()).size, get_token_string(peek_type()).data);
    }
}

static expect_expr_ptr prefix_parse_expression(const token tok) {


    switch (tok.type) {

    // case MINUS: {
    //     advance_and_check_ret_obj("No values after - prefix");
    //     UP<object> right = parse_expression(LOWEST);
    //     if (right->type() == ERROR_OBJ) {
    //         return right; }

    //     return UP<object>(new prefix_expression_object(MAKE_UP(operator_object, NEGATE_OP), std::move(right)));
    // } break;
    case INT_LIT: {
        advance_and_check_ret_expect_expr_ptr("No values after integer prefix");

        const integer_constant_node int_const_nd = make_integer_constant_node_with_str(tok.data);

        push_g_node_stack((node_ptr){(uint8_t*) &int_const_nd, false});

        const node_stack_ptr stack_ptr = get_top_of_g_node_stack();
        if (stack_ptr.is_none) {
            FATAL_ERROR("bruh"); }

        const expect_expr_ptr expect = make_expect_expr_ptr(stack_ptr.addr);

        return expect; 
    } break;
    // case STR_LIT:
    //     return parse_string_literal(tok);
    case OPEN_PAREN: {
        advance_and_check_ret_expect_expr_ptr("No values after open parenthesis in expression");
        const expect_expr_ptr expr = parse_expression(LOWEST, peek());
        if (peek_type() != CLOSE_PAREN) {
            push_err_ret_expect_expr_ptr("Expected closing parenthesis, got (%.*s)", (int) get_token_string(peek_type()).size, get_token_string(peek_type()).data);}
        advance_and_check_ret_expect_expr_ptr("No values after close parenthesis in expression");
        return expr;
    } break;
    // case LESS_THAN:
    //     advance_and_check_ret_obj("No values after less than in expression");
    //     if (token_position < tokens.size()) { // NOT CHECKED !!
    //         if (peek_type() == EQUAL) {
    //             advance_and_check_ret_obj("No values after <= in expression");
    //             return UP<object>(new operator_object(LESS_THAN_OR_EQUAL_TO_OP));
    //         }
    //     }
    //     return UP<object>(new operator_object(LESS_THAN_OP));
    case LINE_END:
        FATAL_ERROR_STACK_TRACE("Received LINE_END token, either parser error or malformed query");
    default:
        push_err_ret_expect_expr_ptr("No prefix function for (%.*s)", (int) get_token_string(peek_type()).size, get_token_string(peek_type()).data);
    }
}

static expect_expr_ptr parse_expression(const size_t precedence, const token tok) {
    
    // std::cout << "parse_expression called with " << token_type_to_string(peek_type()) << std::endl;

    expect_expr_ptr left = prefix_parse_expression(tok);
    if (left.is_error) {
        return left;
    }

    while (peek_type() != LINE_END &&
           peek_type() != CLOSE_PAREN &&
           peek_type() != SEMICOLON &&
        //    peek_type() != COMMA && 
           precedence < numeric_precedence(peek_type()) ) {
            const expect_expr_ptr prev_left = left; // Having prev_left allows for no-ops
            // const expect_expr_ptr left = infix_parse_expression(left);
            // if (left.is_error) {
            //     return left;
            // } 

            if (left.expression.addr == prev_left.expression.addr) {
                break; 
            }

            break;
        }     
        
    return left;
}

static expect_stmt_ptr parse_statement(const token tok) {
    
    // std::cout << "parse_expression called with " << token_type_to_string(peek_type()) << std::endl;

    expect_stmt_ptr left = prefix_parse_statement(tok);
    if (left.is_error) {
        return left;
    }
        
    return left;
}





