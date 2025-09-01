

#include "logger.h"
#include "macros.h"
#include "tac_parse.h"
#include "token.h"
#include "node.h"
#include "node_mem.h"
#include "tac_node.h"
#include "tac_node_mem.h"
#include "asm_node.h"
#include "asm_node_mem.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "files.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>


extern token_stack    g_token_stack;
extern node_stack     g_node_stack;
extern tac_node_stack g_tac_node_stack;
extern tac_node_stack g_instruction_stack;
extern asm_node_stack g_asm_node_stack;


extern bool DEBUG;
extern bool TEST;



typedef struct {
    bool has_cmd_input;
    string input;
    string filename;

    bool end_after_lex;
    bool end_after_parse;
    bool end_after_tac;
    bool end_after_codegen;
    bool end_after_assembly;

    bool debug;
    bool run_tests;
} cmd_line_args;



static int run(const cmd_line_args args);
static cmd_line_args parse_cmd_args(const int argc, char* argv[]);



static cmd_line_args parse_cmd_args(const int argc, char* argv[]) {

    cmd_line_args args = {
        .has_cmd_input      = false,
        .input              = {},
        .filename           = {},
        .end_after_lex      = false,
        .end_after_parse    = false,
        .end_after_tac      = false,
        .end_after_codegen  = false,
        .end_after_assembly = false,

        .debug    = false,
        .run_tests = false
    };

    const size_t skip_program_name = 1;

    const size_t arg_count = (size_t) argc;
    for (size_t i = skip_program_name; i < arg_count; i++) {

        const char* arg = argv[i];

        size_t size = 0;
        while (arg[size++] != '\0') {}
        size--;

        if (size == 2 && strncmp(arg, "-i", 2) == 0) {
            if (args.has_cmd_input) {
                FATAL_ERROR("Command line arguments contain multiple command line inputs"); }

            if (i + 1 >= arg_count) {
                FATAL_ERROR("Command line argument '-i' is missing command line input"); }
            
            i++;

            size = 0;
            while (argv[i][size++] != '\0') {}
            size--;
            
            const string input_to_add = (string){argv[i], size};

            memcpy(&args.input, &input_to_add, sizeof(string));
            args.has_cmd_input = true;

        } else if (size == 2 && strncmp(arg, "-d", 2) == 0) {
            if (args.debug) {
                FATAL_ERROR("Command line arguments contain multiple '-d' flags"); }

            args.debug = true;

        } else if (size == 2 && strncmp(arg, "-t", 2) == 0) {
            if (args.run_tests) {
                FATAL_ERROR("Command line arguments contain multiple '-t' flags"); }

            args.run_tests = true;

        } else if (size == 2 && strncmp(arg, "-S", 2) == 0) {
            if (args.end_after_assembly) {
                FATAL_ERROR("Command line arguments contain multiple '-S' flags"); }

            args.end_after_assembly = true; 

        } else if (size == 5 && strncmp(arg, "--lex", 5) == 0) {
            if (args.end_after_lex) {
                FATAL_ERROR("Command line arguments contain multiple '--lex' flags"); }

            args.end_after_lex = true; 
            
        } else if (size == 7 && strncmp(arg, "--parse", 7) == 0) {
            if (args.end_after_parse) {
                FATAL_ERROR("Command line arguments contain multiple '--parse' flags"); }

            args.end_after_parse = true; 

        } else if (size == 7 && strncmp(arg, "--tacky", 7) == 0) {
            if (args.end_after_tac) {
                FATAL_ERROR("Command line arguments contain multiple '--tacky' flags"); }

            args.end_after_tac = true; 

        } else if (size == 9 && strncmp(arg, "--codegen", size) == 0) {
            if (args.end_after_codegen) {
                FATAL_ERROR("Command line arguments contain multiple '--codegen' flags"); }

            args.end_after_codegen = true;  

        } else {

            if (arg[0] == '-') {
                FATAL_ERROR("Unknown command line flag '%s'", arg); }

            // Assume it is filename
            if (args.has_cmd_input) {
                FATAL_ERROR("Command line arguments contain multiple command line inputs"); }

            const string set_filename = (string){arg, strlen(arg) - 2};
            memcpy(&args.filename, &set_filename, sizeof(string));


            const string file_input = read_file(arg);
            memcpy(&args.input, &file_input, sizeof(string));
            args.has_cmd_input = true;


            // Check it ends in .c
            if (strncmp((arg + size) - 2, ".c", 2) != 0) {
                FATAL_ERROR("File (%.*s) did not end in .c", (int) size, arg); }

            // If errors
            // FATAL_ERROR("Unknown command line argument (%s)", arg);
        }

    }

    return args;
}


typedef struct {
    const string str_val;
    const int    int_val;
} string_int_pair;

static int stack_alloc;

int temp_register_name_to_stack_pos_map(const string name, const bool reset) {

    #define TEMP_REG_MAP_SIZE 1024
    static string_int_pair map[TEMP_REG_MAP_SIZE];
    static size_t cur_size;

    static int stack_pos = 0;

    if (reset) { 
        cur_size = 0; 
        stack_pos = 0;
        stack_alloc = 0;
        return 0;
    }

    bool found = false;
    int ret_val = 0;
    for (size_t i = 0; i < cur_size; i++) {
        if (map[i].str_val.size != name.size) {
            continue; }

        if (strncmp(name.data, map[i].str_val.data, name.size) == 0) {
            found = true;
            ret_val = map[i].int_val;
            break;
        }
    }

    
    if (!found) {
        if (cur_size == TEMP_REG_MAP_SIZE) {
            FATAL_ERROR_STACK_TRACE("Temp register name to stack position map OOM"); }
        
        stack_pos   -= 4;
        stack_alloc += 4;
        const string_int_pair pair = {name, stack_pos};
        memcpy(&map[cur_size], &pair, sizeof(string_int_pair));
        cur_size++;
    
        ret_val = stack_pos;
        return ret_val;
    } else {
        return ret_val;
    }
}




static void replace_unary_op_asm_node_temp_register(unary_op_asm_node* un_op_asm_nd);



static void route_temp_register_replacement(uint8_t* addr) {
    const asm_node_type type = *addr;

    switch (type) {
    case RETURN_ASM_NODE: {
        return_asm_node ret_asm_nd = *(return_asm_node*) addr;
        asm_expr_ptr  expr      = ret_asm_nd.expr;
        route_temp_register_replacement(expr.addr);

        if (ret_asm_nd.expr_contains_dst) {
            move_asm_node mv_asm_nd = ret_asm_nd.move_nd;
    
            register_asm_node* src = mv_asm_nd.src;
            route_temp_register_replacement((uint8_t*) src);
    
            register_asm_node* dst = mv_asm_nd.dst;
            route_temp_register_replacement((uint8_t*) dst);            
        }
    } break;
    case UNARY_OP_ASM_NODE: {
        replace_unary_op_asm_node_temp_register((unary_op_asm_node*) addr);
    } break;
    case REGISTER_ASM_NODE: {
        register_asm_node reg_asm_nd = *(register_asm_node*) addr;
        if (!reg_asm_nd.is_temp) {
            break; }

        const int stack_loc = temp_register_name_to_stack_pos_map(reg_asm_nd.temp_reg.name, false);

        const stack_loc_asm_node stack_loc_asm_nd = make_stack_loc_asm_node(stack_loc);

        const register_asm_node stack_reg = make_register_asm_node_with_stack_loc_dont_place_on_stack(stack_loc_asm_nd); // TODO If changing reg_asm_nodes to values, have to change this line and the next. & of * is bad

        memcpy(addr, &stack_reg, sizeof(register_asm_node));
    } break;

    // Don't care list
    case INT_IMMEDIATE_ASM_NODE:
        break;

    default: FATAL_ERROR("Thing I couldn't change (%.*s)", (int) get_asm_node_string(type).size, get_asm_node_string(type).data);
    }
}

static void replace_unary_op_asm_node_temp_register(unary_op_asm_node* un_op_asm_nd) {

    if (un_op_asm_nd->has_prev) {
        route_temp_register_replacement(un_op_asm_nd->prev.addr); }

    register_asm_node* dst_reg_asm_nd = un_op_asm_nd->dst_reg;
    if (dst_reg_asm_nd->is_temp) {
        const int stack_loc = temp_register_name_to_stack_pos_map(dst_reg_asm_nd->temp_reg.name, false);
    
        const stack_loc_asm_node stack_loc_asm_nd = make_stack_loc_asm_node(stack_loc);
    
        const register_asm_node stack_reg = make_register_asm_node_with_stack_loc_dont_place_on_stack(stack_loc_asm_nd);
    
        memcpy(dst_reg_asm_nd, &stack_reg, sizeof(register_asm_node));
    }

    asm_expr_ptr src_expr = un_op_asm_nd->src;
    route_temp_register_replacement(src_expr.addr);
}

[[nodiscard]] static bool replace_temp_register_variables_with_stack_locations(const function_asm_node func) {

    const bool reset = true;
    temp_register_name_to_stack_pos_map((string){}, reset);

    const size_t  size   = func.nodes.nodes.size;
    asm_node_ptr* values = func.nodes.nodes.values;

    for (size_t i = 0; i < size; i++) {
        route_temp_register_replacement(values[i].addr);
    }

    return true;
}



[[nodiscard]] static register_asm_node* get_scratch_register() {
    return make_register_asm_node_with_x86(EAX);
}

static void route_stack_replacement(uint8_t* addr) {

    const asm_node_type type = *addr;

    switch (type) {
        case RETURN_ASM_NODE: {
        return_asm_node ret_asm_nd = *(return_asm_node*) addr;
        asm_expr_ptr expr = ret_asm_nd.expr;
        route_stack_replacement(expr.addr);

        // move_asm_node mv_asm_nd = ret_asm_nd.move_nd;

        // register_asm_node* src = mv_asm_nd.src;
        // route_temp_register_replacement((uint8_t*) src);

        // register_asm_node* dst = mv_asm_nd.dst;
        // route_temp_register_replacement((uint8_t*) dst);

    } break;
    case UNARY_OP_ASM_NODE: {
        unary_op_asm_node* un_op_asm_nd = (unary_op_asm_node*) addr;

        // If it doesn't have a previous, then it's something like 'movl $0, -4(%rbp)', so there's no issue
        if (!un_op_asm_nd->has_prev) {
            break; }


        
        asm_expr_ptr src = un_op_asm_nd->src; 
        const asm_node_type src_type = *src.addr;
        // Because it has previous, assume that it is a register/location type
        if (src_type != REGISTER_ASM_NODE) {
            FATAL_ERROR("Unary op source was not register type"); }

        // Generate intermediate step of loading src into scratch register
        register_asm_node* intermediate_dst  = get_scratch_register();
        register_asm_node* intermediate_src  = (register_asm_node*) src.addr;
        move_asm_node      intermediate_move = make_move_asm_node(intermediate_src, intermediate_dst);



        asm_node_stack_ptr stack_mv_ptr = place_on_asm_node_stack((uint8_t*) &intermediate_move);

        asm_node_linked_ptr link = make_asm_node_linked_ptr(un_op_asm_nd->prev, (asm_expr_ptr){stack_mv_ptr.addr});

        asm_node_stack_ptr stack_link_ptr = place_on_asm_node_stack((uint8_t*) &link);

        un_op_asm_nd->prev = (asm_expr_ptr){stack_link_ptr.addr};

        un_op_asm_nd->src = (asm_expr_ptr) {(uint8_t*) intermediate_dst};

    } break;

    // Don't care list
    case INT_IMMEDIATE_ASM_NODE: case REGISTER_ASM_NODE:
        break;

    default: FATAL_ERROR("Thing I couldn't change (%.*s)", (int) get_asm_node_string(type).size, get_asm_node_string(type).data);
    }

}

// Currently only changes move instructions
[[nodiscard]] static bool replace_double_stack_operations_with_scratch_register(const function_asm_node func) {

    const size_t  size   = func.nodes.nodes.size;
    asm_node_ptr* values = func.nodes.nodes.values;

    for (size_t i = 0; i < size; i++) {
        route_stack_replacement(values[i].addr);
    }

    return true;
}



#define RUN_ERROR(fmt, ...)     \
    do {                        \
    fprintf(stderr, "RUN ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    return EXIT_FAILURE;    \
    } while(0)


static int run(const cmd_line_args args) {

    if (!args.run_tests && !TEST && args.debug) {
        DEBUG = true; }

    // Reset global objects
    g_token_stack.top       = 0;
    g_node_stack.top        = 0;
    g_tac_node_stack.top    = 0;
    g_asm_node_stack.top    = 0;


    const bool lex_ok = lex(&args.input);
    if (!lex_ok) { RUN_ERROR("Lex fail"); }

    if (unlikely(DEBUG)) { print_g_tokens(); }

    if (g_token_stack.top == 0) { RUN_ERROR("No tokens generated"); }

    if (args.end_after_lex) { return EXIT_SUCCESS; }


    const expect_program_node expect_program_ast = parse();
    if (expect_program_ast.is_error) { RUN_ERROR("Parse fail"); }

    const program_node program_ast = expect_program_ast.node;

    if (unlikely(DEBUG)) { print_g_nodes(); }

    if (unlikely(DEBUG)) { print_program_ast(program_ast); }

    if (g_node_stack.top == 0) { RUN_ERROR("No nodes generated"); }

    if (args.end_after_parse) { return EXIT_SUCCESS; }

    const expect_program_tac_node expect_tac_program = parse_ast(program_ast);
    if (expect_tac_program.is_error) { RUN_ERROR("Tac parse fail"); }

    const program_tac_node tac_program = expect_tac_program.node;

    if (unlikely(DEBUG)) { print_g_tac_nodes(); }

    if (unlikely(DEBUG)) { print_tac_program(tac_program); }

    if (g_tac_node_stack.top == 0) { RUN_ERROR("No tac nodes generated"); }

    if (args.end_after_tac) { return EXIT_SUCCESS; }


    const expect_program_asm_node expect_asm_prog = codegen(tac_program);
    if (expect_asm_prog.is_error) { RUN_ERROR("Codegen fail"); }

    program_asm_node asm_prog = expect_asm_prog.node;

    if (unlikely(DEBUG)) { print_g_asm_nodes((string){}); }

    if (unlikely(DEBUG)) { print_asm_program(asm_prog, (string){"Before temp replacement ---", 27}); }

    if (g_asm_node_stack.top == 0) { RUN_ERROR("No asm_nodes generated"); }

    // Maybe add a flag to stop here?

    const bool temp_replace_rc = replace_temp_register_variables_with_stack_locations(asm_prog.main_function);
    if (!temp_replace_rc) {
        RUN_ERROR("Failed to replace temp registers"); }

    if (unlikely(DEBUG)) { print_g_asm_nodes((string){"After temp replacement ---", 26}); }

    if (unlikely(DEBUG)) { print_asm_program(asm_prog, (string){"After temp replacement ---", 26}); }

    const int round_stack_alloc =  stack_alloc != 0 ? (8 - stack_alloc % 8) : 0;
    asm_prog.main_function.stack_allocate_ammount = stack_alloc + round_stack_alloc;


    const bool double_replace_rc = replace_double_stack_operations_with_scratch_register(asm_prog.main_function); // <--- Here
    if (!double_replace_rc) {
        RUN_ERROR("Failed to replace double stack operations"); }

    if (unlikely(DEBUG)) { print_g_asm_nodes((string){"After double stack operations replacement ---", 45}); }

    if (unlikely(DEBUG)) { print_asm_program(asm_prog, (string){"After double stack operations replacement ---", 45}); }



    if (args.end_after_codegen) { return EXIT_SUCCESS; }
    


    const string assembly = get_assembly(asm_prog);
    
    
    const string filename = args.filename.size == 0 ? (string){"main", 4} : args.filename;

    const bool ok = write_file(filename, assembly);
    if (!ok) { RUN_ERROR("Failed to write to assembly file"); } 

    if (unlikely(DEBUG)) { 
        printf("Assembly written to %.*s.s\n", (int) filename.size, filename.data); }

    if (args.end_after_assembly) {
        if (unlikely(DEBUG)) { printf("Compilation finished at assembly generation.\n"); }
        return EXIT_SUCCESS;
    }



    char gcc_cmd_buffer[512];

    snprintf(gcc_cmd_buffer, 512, "gcc \"%.*s.s\" -o \"%.*s\"", 
        (int) args.filename.size, args.filename.data,
        (int) args.filename.size, args.filename.data
    );

    printf("Compile command: %s\n", gcc_cmd_buffer);

    system(gcc_cmd_buffer);

    // const int compile_rc = compile_with_gcc(args.filename);
    // if (compile_rc != EXIT_SUCCESS) {
    //     RUN_ERROR("Failed to compile assembly file into executable"); }

    if (unlikely(DEBUG)) { printf("Compiled file written to %.*s\n", (int) args.filename.size, args.filename.data); }

    return EXIT_SUCCESS;
}




#define TEST_RUN_ERROR(fmt, ...)     \
    do {                        \
    fprintf(stderr, "TEST RUN ERROR at %s:%d:%s " fmt "\n%s", __FILE__, __LINE__, RED() __VA_OPT__(,) __VA_ARGS__, RESET()); \
    return EXIT_FAILURE;    \
    } while(0)

static int run_tests([[maybe_unused]] const cmd_line_args g_args, const char* test_folder) {

    TEST = true;

    if (g_args.debug) {
        DEBUG = true; }

    printf("Running tests from (%s)\n", test_folder);

    const size_t file_capcity = 64;

    string* filenames     = calloc(file_capcity, sizeof(string));
    if (!filenames) { FATAL_ERROR_STACK_TRACE("calloc() failed: %s", strerror(errno)); }

    string* files         = calloc(file_capcity, sizeof(string));
    if (!filenames) { FATAL_ERROR_STACK_TRACE("calloc() failed: %s", strerror(errno)); }

    bool* expect_fail_ids = calloc(file_capcity, sizeof(bool));
    if (!filenames) { FATAL_ERROR_STACK_TRACE("calloc() failed: %s", strerror(errno)); }

    const folder_storage storage = get_tests(test_folder, file_capcity, expect_fail_ids, filenames, files);

    // for (size_t i = 0; i < file_capcity; i++) {
    //     printf("%s\n", expect_fail_ids[i] ? "EXPECT_FAIL" : "NOT");
    // }

    if (storage.num_files == 0) {
        TEST_RUN_ERROR("Test folder (%s) contained no tests", test_folder); }



    const string expect_fail_str = (string){"[[expect_fail]]", 15};

    const char padding[64] = "----------------------------------------------------------------";

    size_t tests_passed = 0;

    const size_t MAX_TESTS = storage.num_files;

    for (size_t i = 0; i < storage.num_files && i < MAX_TESTS; i++) {
        const string filename_str    = storage.filenames[i];
        const string file_output_str = storage.file_outputs[i];

        if (strncmp((filename_str.data + filename_str.size) - 2, ".c", 2) != 0) {
            TEST_RUN_ERROR("Test file (%.*s) did not end in .c", (int) filename_str.size, filename_str.data); }

        const int line_size = 64;

        const bool  expect_fail      = expect_fail_ids[i];
        const char* expect_fail_data = expect_fail ? expect_fail_str.data : "";
        
        const int expect_fail_size = expect_fail ? expect_fail_str.size : 0;

        int pad_size = line_size - (int) filename_str.size - expect_fail_size - (expect_fail ? 1 : 0);
        if (pad_size < 2) { pad_size = 2; }

        printf("%.*s %.*s%.*s%.*s\n", 
            (int) filename_str.size, filename_str.data,
            (int) expect_fail_size, expect_fail_data,
            (int) expect_fail ? 1 : 0, " ",
                  pad_size, padding);

        const string input = file_output_str;

        const cmd_line_args args = {
            .has_cmd_input      = true,
            .input              = input,
            .filename           = (string){filename_str.data, filename_str.size-2},
            .end_after_lex      = g_args.end_after_lex,
            .end_after_parse    = g_args.end_after_parse,
            .end_after_tac      = g_args.end_after_tac,
            .end_after_codegen  = g_args.end_after_codegen,
            .end_after_assembly = g_args.end_after_assembly,

            .debug     = false,
            .run_tests = false
        };

        const int rc = run(args);

        const size_t test_result_size = 9;
        if (rc == EXIT_FAILURE && expect_fail) {
            printf("%sTEST PASS%s %.*s\n\n", GREEN(), RESET(), (int) (line_size - test_result_size), padding);
            tests_passed++;
        } else if (rc == EXIT_FAILURE && !expect_fail) {
            printf("Test input:\n'%.*s'\n", (int) file_output_str.size, file_output_str.data);
            printf("%sTEST FAIL%s %.*s\n\n", RED(),   RESET(), (int) (line_size - test_result_size), padding);
        } else if (rc == EXIT_SUCCESS && expect_fail) {
            printf("Test input:\n'%.*s'\n", (int) file_output_str.size, file_output_str.data);
            printf("%sTEST FAIL%s %.*s\n\n", RED(),   RESET(), (int) (line_size - test_result_size), padding);
        } else if (rc == EXIT_SUCCESS && !expect_fail) {
            printf("%sTEST PASS%s %.*s\n\n", GREEN(), RESET(), (int) (line_size - test_result_size), padding);
            tests_passed++;
        }
        
    }


    printf("%s%zu%s / %zu tests passed\n\n", GREEN(), tests_passed, RESET(), storage.num_files);


    for (size_t i = 0; i < storage.num_files; i++) {
        free((void*) storage.filenames[i].data);
        free((void*) storage.file_outputs[i].data);
    }

    free(filenames);
    free(files);
    free(expect_fail_ids);
    
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {

    const cmd_line_args g_args = parse_cmd_args(argc, argv);

    printf("Command line arguments {\n"
        "  .has_cmd_input      = %s,\n"
        "  .input              = '%.*s',\n"
        "  .end_after_lex      = %s,\n"
        "  .end_after_parse    = %s,\n"
        "  .end_after_tac      = %s,\n"
        "  .end_after_codegen  = %s,\n"
        "  .end_after_assembly = %s,\n"
        "  .debug     = %s,\n"
        "  .run_tests = %s\n"
    "}\n\n",
        g_args.has_cmd_input      ? "true" : "false",
        (int) g_args.input.size,  g_args.input.data,
        g_args.end_after_lex      ? "true" : "false",
        g_args.end_after_parse    ? "true" : "false",
        g_args.end_after_tac      ? "true" : "false",
        g_args.end_after_codegen  ? "true" : "false",
        g_args.end_after_assembly ? "true" : "false",
        g_args.debug              ? "true" : "false",
        g_args.run_tests          ? "true" : "false"
    );

    if (unlikely(g_args.has_cmd_input && g_args.debug)) {
        printf("Command line input:\n'%.*s'\n", (int) g_args.input.size, g_args.input.data);
    }

    if (g_args.run_tests) {
        const char* test_folder = "tests";
        return run_tests(g_args, test_folder);
    }

    return run(g_args);
}






