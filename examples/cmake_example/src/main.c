#include <stdio.h>
#include "used_file.h"
#include "helper.h"

int main() {
    printf("Example Application\n");
    printf("===================\n\n");
    
    // These files will be tracked
    used_function();
    helper_function();
    
    printf("\nApplication completed successfully.\n");
    printf("Check the build reports to see which files were accessed.\n");
    
    return 0;
}
