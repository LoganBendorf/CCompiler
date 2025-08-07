

#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



const char* GREEN()  { return "\x1b[32m"; }
const char* YELLOW() { return "\x1b[33m"; }
const char* RED()    { return "\x1b[31m"; }
const char* RESET()  { return "\x1b[0m";  }


[[maybe_unused]] bool in_debug() {
    static char buf[4096];
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

void format_str( char* input) {

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
    const size_t frame_count = (size_t) backtrace(frames, 20);

    strings = backtrace_symbols(frames,  (int) frame_count);

    const size_t junk_size = 3; 
    const size_t iterations = frame_count > junk_size ? frame_count - junk_size : 0;
    
    fprintf(stderr, "Stack trace (%zu frames):\n", iterations);
    for (size_t i = 0; i < iterations; i++) {
        format_str(strings[i]);
        fprintf(stderr, "  %s\n", strings[i]);
    }
    
    free(strings);
}


#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libunwind.h>
#include <inttypes.h>
#include <sys/wait.h>



static int max_i(const int a, const int b) {
    return a > b ? a : b;
}

void format_unwind_str(char* input) {

    [[maybe_unused]] const size_t str_size = strlen(input);

    size_t at_index = 0;
    while (strncmp(input + at_index, "at", 2) != 0) {
        at_index += 1;
    }
    at_index += 2;

    size_t source_index = at_index;
    const char* source_str = "source";
    while (strncmp(input + source_index, source_str, strlen(source_str)) != 0) {
        source_index += 1;
    }

    // lol
    input[at_index - 3] = '(';
    input[at_index - 2] = ')';

    input[at_index - 1] = ' ';
    input[at_index - 0] = '\n';
    input[at_index + 1] = '\t';
    input[at_index + 2] = 'a';
    input[at_index + 3] = 't';
    input[at_index + 4] = ' ';


    

    const size_t left_index = at_index + 5;

    const size_t right_index = source_index - 1;

    size_t i = 0;
    for (; i + right_index < str_size; i++) {
        input[i + left_index] = input[i + right_index];
    }

    input[i + left_index] = 0;
}

// Method 4: Fixed addr2line with better address handling
void print_stack_unwind_addr2line_fixed(void) {
    unw_context_t ctx;
    unw_cursor_t  cursor;
    unw_word_t    ip;
    
    unw_getcontext(&ctx);
    unw_init_local(&cursor, &ctx);
    
    fprintf(stderr, "Stack trace ====================================\n");
    
    // Get the executable path
    static char exe_path[1024];
    const ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        fprintf(stderr, "Could not get executable path\n");
        return;
    }
    exe_path[len] = '\0';
    
    // Collect all frames first
    #define MAX_FRAMES 32
    uintmax_t frames[MAX_FRAMES];
    int total_frames = 0;
    
    while (unw_step(&cursor) > 0 && total_frames < MAX_FRAMES) {
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        frames[total_frames++] = (uintmax_t)ip;
    }
    
    // Skip the last 3 frames (libc_start_main, _start, etc.)
    const int skip_setup_frames = 3;
    const int frames_to_show = max_i(total_frames - skip_setup_frames, 0);
    
    // Print the frames we want to show
    const int skip_this_func = 1;
    for (int frame_num = skip_this_func; frame_num < frames_to_show; frame_num++) {
        ip = frames[frame_num];
        
        // For return addresses, we want the previous instruction
        const uintmax_t addr_to_lookup = ip - 1;
        
        static char cmd[2048];
        snprintf(cmd, sizeof(cmd), "addr2line -e %s -f -C -i -p 0x%jx 2>/dev/null", 
                 exe_path, addr_to_lookup);
        
        FILE *fp = popen(cmd, "r");
        if (fp) {
            static char result[1024];
            if (fgets(result, sizeof(result), fp)) {
                result[strcspn(result, "\n")] = 0;  // Remove newline
                
                // Prints here if correct
                if (strstr(result, "??") == NULL && strstr(result, ":0") == NULL) {

                    format_unwind_str(result);
                    fprintf(stderr, "  %d. %s\n", frame_num, result);

                    pclose(fp);
                    continue;
                }
            }
            pclose(fp);
        }
        
        // Fallback to symbol info
        Dl_info dlinfo;
        if (dladdr((void*)ip, &dlinfo) && dlinfo.dli_sname) {
            uintmax_t offset = ip - (uintmax_t)dlinfo.dli_saddr;
            fprintf(stderr, "  #%d 0x%jx in %s+0x%jx\n",
                    frame_num, ip, dlinfo.dli_sname, offset);
        } else {
            fprintf(stderr, "  #%d 0x%jx in ??\n", frame_num, ip);
        }
    }
}

// Convenience wrapper - use this in your code
void print_stack_unwind(void) {
    // Try the hybrid approach for best results
    print_stack_unwind_addr2line_fixed();
}
