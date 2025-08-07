
#include "token.h"

#include "logger.h"
#include "structs.h"

#include <stdint.h>
#include <string.h>


extern bool DEBUG;
extern bool TEST;



string_span token_span() {
    static const string arr[] = {
                     {"NULL_TOKEN", 10}, {"OPEN_PAREN", 10}, {"CLOSE_PAREN", 11}, 
                     {"OPEN_BRACE", 10}, {"CLOSE_BRACE", 11},
                      {"INT_LIT", 7}, {"IDENTIFIER", 10}, {"SEMICOLON", 9}, {"LINE_END", 8}, {"C_TYPE", 6},
                      {"DECREMENT", 9}, {"BITWISE_COMPLEMENT", 18}, {"MINUS", 5},
                        {"main", 4}, {"return", 6}, {"if", 2}, 
                        {"int", 3}, {"void", 4}
                    };
    return (string_span){arr, sizeof(arr) / sizeof(arr[0]) };
}

string get_token_string(const size_t index) {

    const string_span token_strs = token_span();

    if (index >= token_strs.size) {
        FATAL_ERROR_STACK_TRACE("get_token_string(): Out of bounds index, (%zu)", index); }

    return token_strs.strs[index];
}

token_stack g_token_stack;

void print_g_tokens(void) {

    printf("PRINTING TOKENS ------------------\n");
    const size_t num_toks = g_token_stack.top;

    char str[32];
    sprintf(str, "%zu", num_toks);
    const size_t max_num_width = strlen(str) + 1; 

    size_t max_type_width = 0;
    for (size_t i = 0; i < num_toks; i++) {

        const string tok_type_str = get_token_string(g_token_stack.stack[i].type);
        max_type_width = tok_type_str.size > max_type_width ? tok_type_str.size : max_type_width;
    }

    for (size_t i = 0; i < num_toks; i++) {

        const string tok_type_str = get_token_string(g_token_stack.stack[i].type);
        const string tok_data_str = g_token_stack.stack[i].data;
        

        printf("%*zu: Type (%.*s)%*s'%.*s'\n", 
            (int) max_num_width,                        i + 1, 
            (int) tok_type_str.size,                    tok_type_str.data, 
            (int) (max_type_width - tok_type_str.size), "", 
            (int) tok_data_str.size,                    tok_data_str.data
        );
    }
    printf("DONE -----------------------------\n\n");
}







const token* g_token_stack_top_ptr() {
    const token* addr = &g_token_stack.stack[g_token_stack.top];
    return addr;
}


token pop_g_token_stack( void ) {
    if (g_token_stack.top == 0) {
        return (token){LINE_END, {}, 0, 0};
    }

    g_token_stack.top -= 1;
    const token ret_val = g_token_stack.stack[g_token_stack.top];
    return ret_val;
}

void push_g_token_stack(token tok) {

    const size_t size = 1;
    if (size + g_token_stack.top > TOKEN_STACK_SIZE) {
        FATAL_ERROR_STACK_TRACE("Stack OOM"); }
    
    // Bruh const is dumb
    memcpy(&g_token_stack.stack[g_token_stack.top], &tok, sizeof(token));
    
    g_token_stack.top += size;
}








