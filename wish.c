#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "batch.h"

int main(int argc, char *argv[]) {
    FILE *in = stdin;
    
    // batch mode check. if the arguments coming is at least 2, e.g. "./wish batch_file.txt"
    // where ./wish is argument #1 and batch_file.txt is argument #2 therefore the count or argc will
    // equal two then call the batch method.
    if (argc == 2) {
        return runBatchFile(argv[1]);
    }

    char *line = NULL;         // buffer that getline() will allocate/resize
    size_t cap = 0;            // capacity hint for getline()

    // main loop (run until "exit" or EOF)
    while (1) {
        if (in == stdin) {
            fputs("wish> ", stdout);
            fflush(stdout);
        }

        // read one line (includes trailing '\n' if any); returns -1 on EOF
        ssize_t n = getline(&line, &cap, in);
        if (n == -1) {
            free(line);
            exit(0);
        }

        // drop the trailing newline, if present
        if (n > 0 && line[n - 1] == '\n') line[n - 1] = '\0';

        // ignore empty lines (just pressing Enter)
        if (line[0] == '\0') continue;

        // built-in: exit
        if (strcmp(line, "exit") == 0) {
            free(line);
            exit(0);
        }

        // placeholder: later, parse with strsep() and exec via fork/execv
        // this is a test print out to confirm its taking in what you are typing
        printf("you typed: %s\n", line);
    }
}
