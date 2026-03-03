#include <stdio.h>
#include "math_ops.h"
#include "utils.h"

int main() {
    printf("Makefile Example Application\n");
    printf("============================\n\n");
    
    int a = 10, b = 5;
    
    printf("Math operations:\n");
    printf("  %d + %d = %d\n", a, b, add(a, b));
    printf("  %d * %d = %d\n", a, b, multiply(a, b));
    
    printf("\nUtility functions:\n");
    print_hello();
    
    printf("\nApplication completed.\n");
    printf("Note: unused_ops.c was NOT compiled or accessed.\n");
    printf("Check build reports to verify file tracking.\n");
    
    return 0;
}
