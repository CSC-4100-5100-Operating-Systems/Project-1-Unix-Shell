// wish.c — minimal starter shell
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    FILE *in = stdin;          // read commands from standard input (keyboard)

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
