

#include "helpers.h"

#include "structs.h"
#include "node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int atoin(const string str) {
    if (str.data == nullptr || str.size == 0) { return 0; }
    if (str.size >= 16) { return 0; }
    
    static char temp[16];
    
    strncpy(temp, str.data, str.size);
    temp[str.size] = '\0';
    
    int result = atoi(temp);
    
    return result;
}

void debug_print_ast_node(const uint8_t* addr) {

    if (addr == 0) {
        printf("debug_print_ast_node() received nullptr\n"); }

    char buf[1024];
    const size_t size = node_addr_to_str(addr, buf, 1024, 0);
    
    const node_type type = *addr;
    const string node_str = get_node_string(type);
    printf("Printing AST node. Type: %.*s. Value: %.*s\n", (int) node_str.size, node_str.data, (int) size, buf);
}


size_t allign(const size_t size) {
    const size_t to_add = size % GLOBAL_STACK_ALLIGNMENT;
    return size + to_add;
}










