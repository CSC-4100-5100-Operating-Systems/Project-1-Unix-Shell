#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    char input[1024];

    while (1) {
        
        // Creating the "wish> " prompt
        fputs("wish> ", stdout);
        fflush(stdout);

        // This places a newline so the prompt will move down if there is an input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            putchar('\n');
            break;
        }

        // Removes the newline at the end of the incoming line and replaces is with null
        input[strcspn(input, "\n")] = '\0';

        // Lastly this will ignore empty line e.g. just pressing enter
        if (input[0] == '\0') {
            continue;
        }
        
        printf("you typed: %s\n", input);
    }

    return 0;
}
