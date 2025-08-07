

#include "lexer.h"

#include "logger.h"
#include "structs.h"
#include "token.h"

#include <complex.h>
#include <ctype.h>
#include <float.h>
#include <stddef.h>
#include <string.h>



#define LOG_ERR(fmt, ...) \
    do { \
    fprintf(stderr, "CODEGEN ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    } while(0)



typedef struct {
    const bool is_error;
    union {
        const size_t size_t_value;
        const string error_msg;
    };
} expect_size_t;


extern bool DEBUG;
extern bool TEST;

extern token_stack g_token_stack;



static const string* input;
static size_t input_pos;

static size_t line;
static const char* line_start;
static size_t col;



static char cur_char()  { return input->data[input_pos]; }
static char next_char() { return input->data[input_pos + 1]; }


static void add_token(const token_type type, const string data) {
    push_g_token_stack((token){type, data, line, col});
}

size_t chars_till_line_end() {
    size_t pos = 0;
    while (line_start[pos] != '\0' && line_start[pos] != '\n') {
        pos++;
    }
    return pos;
}



// Should be dumb, complicated numbers should be constructed in the PARSER (i.e decimals, negatives)
static expect_size_t read_number() {

    bool num_was_read = false;
    const size_t start = input_pos;

    while (input_pos < input->size && isdigit(cur_char())) {
        num_was_read = true;
        input_pos++;
    }

    if (input_pos < input->size && isalpha(cur_char())) {
        LOG_ERR("Invalid identifier, can not start with number, line '%.*s'", (int) chars_till_line_end(), line_start);
        return (expect_size_t){.is_error=true};
    }

    if (num_was_read) {
        return (expect_size_t){.is_error=false, {start}};
    } else {
        return (expect_size_t){.is_error=true};
    }
}

static size_t read_string() {
    const size_t start = input_pos;
    while (input_pos < input->size && ( isalpha(cur_char()) || cur_char() == '_' || isdigit( cur_char() ))) {
        input_pos++;
    }
    //printf("start [%d], pos after read [%d]. ", start, input_position);
    //std::string word = input.substr(start, input_position - start);
    //std::cout << "word = " + word + "\n";
    return start;
}


bool lex(const string* set_input) {
    input = set_input;
    input_pos = 0;
    line = 0;
    line_start = input->data;
    col = 0;

    while (input_pos < input->size) {


    switch (cur_char()) {
    // Whitespace
    case '\n': 
        line++; 
        col = 0; 
        input_pos++; 
        line_start = input->data + input_pos; 
        break;
    case ' ': col++; input_pos++; break;
    case '\t': col+= 4 /*???*/; input_pos++; break;

    case '-': {
        input_pos++;
        if (input_pos < input->size && cur_char() == '-') {
            add_token(DECREMENT, (string){"--", 2});
            input_pos++;
            col++;
        } else {
            add_token(MINUS, (string){"-", 1});
        } 
    } break;
    case '~': {
        add_token(BITWISE_COMPLEMENT, (string){"~", 1});
        input_pos++;
        col++;
    } break;
    case '/': {
        if (input_pos + 1 >= input->size) {
            LOG_ERR("Singular '/' on line '%.*s'", (int) chars_till_line_end(), line_start);
            return false;
        } 

        input_pos++;

        bool multi_line_comment_terminated = true;
        if (cur_char() == '/' ) {
            while (cur_char() != '\0' && cur_char() != '\n') {
                input_pos++;
            }
        } else if (cur_char() == '*') {
            if (input_pos + 1 >= input->size) {
                LOG_ERR("'/**/' comment not terminated, started on line '%.*s'", (int) chars_till_line_end(), line_start);
                return false;
            } 

            while (!(cur_char() == '*' && next_char() == '/')) {
                if (cur_char() == '\0' || next_char() == '\0') {
                    multi_line_comment_terminated = false;
                    break; 
                }
                input_pos++;
            }

            input_pos += 2;
        } else {
            LOG_ERR("Singular '/' on line '%.*s'", (int) chars_till_line_end(), line_start);
            return false;
        }

        if (!multi_line_comment_terminated) {
            LOG_ERR("'/**/' comment not terminated, started on line '%.*s'", (int) chars_till_line_end(), line_start);
            return false;
        }

    } break;
    case ';': 
        add_token(SEMICOLON, (string){";", 1}); input_pos++; break;
    case '(': 
        add_token(OPEN_PAREN, (string){"(", 1}); input_pos++; break;
    case ')': 
        add_token(CLOSE_PAREN, (string){")", 1}); input_pos++; break;
    case '{': 
        add_token(OPEN_BRACE, (string){"{", 1}); input_pos++; break;
    case '}': 
        add_token(CLOSE_BRACE, (string){"}", 1}); input_pos++; break;

    default: {
        if (isalpha(cur_char())) {
            const size_t start = read_string();
            if (input_pos == start) {
                LOG_ERR("Failed to read string, %s", input->data + start);
                return false;
            }

            const string word = {input->data + start, input_pos - start};

            bool is_keyword = false;
            const string_span token_strs = token_span();
            for (size_t i = KEYWORD_START; i < KEYWORD_END; i++) {

                const string token_str = token_strs.strs[i];
                if (token_str.size == word.size && strncmp(token_str.data, word.data, word.size) == 0) {
                    add_token((token_type) i, word);
                    col += word.size;
                    is_keyword = true;
                    break;
                }
            }
            
            if (is_keyword) {
                continue; }

            add_token(IDENTIFIER, word);
            col += word.size;

        } else if (isdigit(cur_char())) {
            const expect_size_t expect_start = read_number();
            if (expect_start.is_error) {
                LOG_ERR("Invalid number. Line = %zu , column = %zu", line, col); 
                return false;
            }

            const size_t start = expect_start.size_t_value;

            const string word = {input->data + start, input_pos - start};

            add_token(INT_LIT, word);
            col += word.size;
        } else {
            LOG_ERR("Illegal token (%c) on line '%.*s'", cur_char(), (int) chars_till_line_end(), line_start);
            return false;
        }
    }

    }
    }

    return true;
}

















