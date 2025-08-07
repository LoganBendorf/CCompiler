

#include "unary_op.h"

#include "logger.h"
#include "structs.h"

extern bool TEST;



[[nodiscard]] static string_span unary_op_span() {
    static const string arr[] = {
                {"NEGATE_OP", 9}, {"BITWISE_COMPLEMENT_OP", 21}, {"PRE_DECREMENT_OP", 16}, {"POST_DECREMENT_OP", 17}
    };
    return (string_span){arr, sizeof(arr) / sizeof(arr[0]) };
}

string get_unary_op_string(const size_t index) {

    const string_span unary_op_strs = unary_op_span();

    if (index >= unary_op_strs.size) {
        FATAL_ERROR_STACK_TRACE("get_unary_op_string(): Out of bounds index, (%zu)", index); }

    return unary_op_strs.strs[index];
}

