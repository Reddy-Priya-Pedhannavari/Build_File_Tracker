#include "unused_file.h"
#include <stdio.h>

// This file exists in Package A but is NEVER used
void unused_function(void) {
    printf("This function from Package A is NEVER called\n");
}
