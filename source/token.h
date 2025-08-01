#pragma once

#include "structs.h"

#include <stddef.h>
#include <stdint.h>

#define TOKEN_STACK_SIZE 1024 * 16


// Dear god if tokens > 255
typedef enum { 
    INT, OPEN_PAREN, CLOSE_PAREN, OPEN_BRACE, CLOSE_BRACE, INT_LIT, IDENTIFIER, SEMICOLON, LINE_END,
    MAIN, VOID, RETURN, IF
}
token_type;
#define KEYWORD_START MAIN

typedef struct {
    const token_type type;
    const string     data;
    const size_t     line;
    const size_t     col;
} token;

typedef struct {
    size_t top;
    token stack[TOKEN_STACK_SIZE];
} token_stack;


string get_token_string(size_t index);
string_span token_span();
void print_g_tokens(void);

token pop_g_token_stack( void );
void  push_g_token_stack(const token tok);

