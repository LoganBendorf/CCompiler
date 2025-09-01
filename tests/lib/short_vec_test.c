
#include "short_vec/short_vec.h"
#include "lib_assert.h"

#include <stdio.h>

extern bool DEBUG;
extern bool TEST;

int main() {
    short_vec_4_int tiny;
    short_vec_4_int_constructor(&tiny);

    S_VEC_PUSH_BACK(tiny, 12);
    lib_assert(S_VEC_INDEX(tiny, 0), 12);

    S_VEC_CLEAR(tiny);
    lib_assert(tiny.size, 0);
    lib_assert(tiny.capacity, 4);

    S_VEC_PUSH_BACK(tiny, 12);
    S_VEC_PUSH_BACK(tiny, 12);
    S_VEC_PUSH_BACK(tiny, 12);
    S_VEC_PUSH_BACK(tiny, 12);
    S_VEC_PUSH_BACK(tiny, 12);
    lib_assert(tiny.size, 5);
    lib_assert(tiny.capacity, 6);

    S_VEC_PUSH_BACK(tiny, 12);
    S_VEC_PUSH_BACK(tiny, 12);
    lib_assert(tiny.size, 7);
    lib_assert(tiny.capacity, 8);

    lib_assert(S_VEC_INDEX(tiny, 6), 12);




    // S_VEC_INSERT(tiny, 99999, 0); // Should crash

    S_VEC_DESTROY(tiny);
}







