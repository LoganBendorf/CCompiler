

#include "structs.h"

#include <stdlib.h>
#include <string.h>

int atoin(const string str) {
    if (str.data == nullptr || str.size == 0) { return 0; }
    if (str.size >= 16) { return 0; }
    
    // Create a temporary null-terminated string
    static char temp[16];
    
    strncpy(temp, str.data, str.size);
    temp[str.size] = '\0';
    
    int result = atoi(temp);
    
    return result;
}










