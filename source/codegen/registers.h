#pragma once

#include "structs.h"

typedef enum { EAX, EBX
} x86_register;

[[nodiscard]] string get_reg_string(const size_t index);


