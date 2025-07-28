

#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



const char* GREEN()  { return "\x1b[32m"; }
const char* YELLOW() { return "\x1b[33m"; }
const char* RED()    { return "\x1b[31m"; }
const char* RESET()  { return "\x1b[0m";  }


[[maybe_unused]] bool in_debug() {
    char buf[4096];
    FILE *status = fopen("/proc/self/status", "r");
    if (!status) return 0;
    
    while (fgets(buf, sizeof(buf), status)) {
        if (strncmp(buf, "TracerPid:", 10) == 0) {
            fclose(status);
            return atoi(buf + 10) != 0;  // Non-zero means debugger attached
        }
    }
    fclose(status);
    return 0;
}

void format_str([[maybe_unused]] char* input) {

    // if (in_debug()) {
    //     return; }

    // fprintf(stderr, "  %s\n", input);


    size_t start = 0;
    const char* program_name = "crapiler";
    while (strncmp(input + start, program_name, strlen(program_name)) != 0) {
        start += 1;
    }

    size_t func_start = start;

    while (input[func_start] != '(' && input[func_start] != '\0') {
        func_start++;
    }

    size_t func_end = func_start;

    while (input[func_end] != '+' && input[func_end] != '\0') {
        func_end++;
    }

    size_t cur_loc = 0;

    // file name
    for (size_t i = 0; i < func_start - start; i++) {
        input[cur_loc++] = input[i + start]; 
    }

    input[cur_loc++] = '/';


    // function name
    for (size_t i = 1; i < func_end - start; i++) {
        input[cur_loc++] = input[i + func_start]; 
    }

    input[func_end - start] = '\0';
}


void print_stack_trace() {
 
    void*  frames[20];
    char** strings;
    const size_t frame_count = backtrace(frames, 20);

    strings = backtrace_symbols(frames, frame_count);

    const size_t junk_size = 3; 
    const size_t iterations = frame_count > junk_size ? frame_count - junk_size : 0;
    
    fprintf(stderr, "Stack trace (%zd frames):\n", iterations);
    for (size_t i = 0; i < iterations; i++) {
        format_str(strings[i]);
        fprintf(stderr, "  %s\n", strings[i]);
    }
    
    free(strings);
}




