
#include <stddef.h>
#include "short_vec.h"

void (*short_vec_v_table(const size_t type_index, const size_t operation_index))(void) {
    switch (type_index) {

    case 0:
        switch (operation_index) {
        case 0:
            return (void (*)(void)) &short_vec_4_int_reserve;
        case 1:
            return (void (*)(void)) &short_vec_4_int_insert;
        case 2:
            return (void (*)(void)) &short_vec_4_int_push_back;
        case 3:
            return (void (*)(void)) &short_vec_4_int_clear;
        case 4:
            return (void (*)(void)) &short_vec_4_int_destroy;
        case 5:
            return (void (*)(void)) &short_vec_4_int_index;
        }

    case 1:
        switch (operation_index) {
        case 0:
            return (void (*)(void)) &short_vec_4_double_reserve;
        case 1:
            return (void (*)(void)) &short_vec_4_double_insert;
        case 2:
            return (void (*)(void)) &short_vec_4_double_push_back;
        case 3:
            return (void (*)(void)) &short_vec_4_double_clear;
        case 4:
            return (void (*)(void)) &short_vec_4_double_destroy;
        case 5:
            return (void (*)(void)) &short_vec_4_double_index;
        }

    case 2:
        switch (operation_index) {
        case 0:
            return (void (*)(void)) &short_vec_4_char_reserve;
        case 1:
            return (void (*)(void)) &short_vec_4_char_insert;
        case 2:
            return (void (*)(void)) &short_vec_4_char_push_back;
        case 3:
            return (void (*)(void)) &short_vec_4_char_clear;
        case 4:
            return (void (*)(void)) &short_vec_4_char_destroy;
        case 5:
            return (void (*)(void)) &short_vec_4_char_index;
        }
   }

   return NULL;
}

