

#include "registers.h"

#include "logger.h"
#include "structs.h"



extern bool DEBUG;
extern bool TEST;



[[nodiscard]] string_span register_span() {
    static const string arr[] = {
                {"%eax", 4}, {"%rbx", 4}
            };
    return (string_span){arr, sizeof(arr) / sizeof(arr[0]) };
}

string get_reg_string(const size_t index) {

    const string_span reg_strs = register_span();

    if (index >= reg_strs.size) {
        FATAL_ERROR_STACK_TRACE("get_reg_string(): Out of bounds index, (%zu)", index); }

    return reg_strs.strs[index];
}






