

#include "logger.h"
#include "node.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

extern token_stack g_token_stack;
extern node_stack  g_node_stack;

typedef struct {
    bool has_cmd_input;
    string input;

    bool end_after_lex;
    bool end_after_parse;
    bool end_after_codegen;
    bool end_after_assembly;
} cmd_line_args;



string read_file(const char* filename) {

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        FATAL_ERROR("Failed to open file: %s (%s)", strerror(errno), filename);
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file);
    if (size > 1024 * 1024 * 10) {
        FATAL_ERROR("File is way too large, size %zumb (%s)", size / (1024 * 1024), filename);
    }
    fseek(file, 0, SEEK_SET);

    if (ferror(file)) {
        fclose(file);
        FATAL_ERROR("Failed to read file: %s (%s)", strerror(errno), filename);
    }

    // Allocate buffer
    char* buffer = malloc(size + 1);
    if (buffer == NULL) {
        fclose(file);
        FATAL_ERROR("Failed to allocate memory for file: %s (%s)", strerror(errno), filename);
    }

    // Read entire file
    fread(buffer, 1, size, file);
    buffer[size] = '\0';

    fclose(file);
    
    return (string){buffer,  size};
}

cmd_line_args parse_cmd_args(const int argc, char* argv[]) {

    cmd_line_args args = {
        .has_cmd_input      = false,
        .input              = {},
        .end_after_lex      = false,
        .end_after_parse    = false,
        .end_after_codegen  = false,
        .end_after_assembly = false
    };

    const size_t skip_program_name = 1;

    const size_t arg_count = (size_t) argc;
    for (size_t i = skip_program_name; i < arg_count; i++) {
        const char* arg = argv[i];

        // if (arg[0] != '-') {
        //     FATAL_ERROR("Malformed command line argument (%s), arguments must start with '-'", arg); }

        size_t size = 0;
        while (arg[size++] != '\0') {}
        size--;

        if (size == 2 && strncmp(arg, "-i", 2) == 0) {
            if (i + 1 >= arg_count) {
                FATAL_ERROR("'-i' missing command line input"); }
            
            i++;

            size = 0;
            while (argv[i][size++] != '\0') {}
            size--;
            
            const string input_to_add = (string){argv[i], size};

            memcpy(&args.input, &input_to_add, sizeof(string));
            args.has_cmd_input = true;

        } else if (size == 2 && strncmp(arg, "-S", 2) == 0) {
            args.end_after_assembly = true; 
        } else if (size == 5 && strncmp(arg, "--lex", 5) == 0) {
            args.end_after_lex = true; 
        } else if (size == 7 && strncmp(arg, "--parse", 7) == 0) {
            args.end_after_parse = true; 
        } else if (size == 9 && strncmp(arg, "--codegen", size) == 0) {
            args.end_after_codegen = true;  
        } else {
            // Assume it is filename
            const string file_input = read_file(arg);
            memcpy(&args.input, &file_input, sizeof(string));
            args.has_cmd_input = true;

            // If errors
            // FATAL_ERROR("Unknown command line argument (%s)", arg);
        }

    }

    return args;
}

int main(int argc, char* argv[]) {


    const cmd_line_args g_args = parse_cmd_args(argc, argv);

    if (g_args.end_after_parse) {
        FATAL_ERROR("Command line argument (--parse) is not supported yet");
    } else if (g_args.end_after_codegen) {
        FATAL_ERROR("Command line argument (--codegen) is not supported yet");
    } else if (g_args.end_after_assembly) {
        FATAL_ERROR("Command line argument (-S) is not supported yet");
    }

    if (g_args.has_cmd_input) {
        printf("Command line input: %.*s\n", (int) g_args.input.size, g_args.input.data);
    }


    lex(&g_args.input);

    print_g_tokens();

    if (g_token_stack.top == 0) {
        FATAL_ERROR("No tokens generated"); }

    if (g_args.end_after_lex) {
        exit(EXIT_SUCCESS); }

    parse();

    print_g_nodes();

    if (g_node_stack.top == 0) {
        FATAL_ERROR("No nodes generated"); }

    if (g_args.end_after_parse) {
        exit(EXIT_SUCCESS); }
    
    // FATAL_ERROR_STACK_TRACE("LOGGER TEST, %d", 69);

    return 0;
}









