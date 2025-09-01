
#include "binary_op.h"

#include "logger.h"
#include "structs.h"

#include <stdio.h>

extern bool TEST;



[[nodiscard]] static string_span binary_op_span() {
    static const string arr[] = {
                {"ADD_OP", 6}, {"SUB_OP", 6}, {"MUL_OP", 6}, {"DIV_OP", 6}, {"MOD_OP", 6}
    };
    return (string_span){arr, sizeof(arr) / sizeof(arr[0]) };
}

string get_binary_op_string(const size_t index) {

    const string_span binary_op_strs = binary_op_span();

    if (index >= binary_op_strs.size) {
        FATAL_ERROR_STACK_TRACE("get_binary_op_string(): Out of bounds index, (%zu)", index); }

    return binary_op_strs.strs[index];
}

