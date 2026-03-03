#include "unused_ops.h"

// These functions are NEVER used
int subtract(int a, int b) {
    return a - b;
}

int divide(int a, int b) {
    return (b != 0) ? (a / b) : 0;
}
