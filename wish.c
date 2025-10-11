#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include "parallel.h"




int main(int argc, char *argv[]) {
    FILE *in = stdin;          // read commands from standard input (keyboard)
  
    // batch mode check. if the arguments coming is at least 2, e.g. "./wish batch_file.txt"
    // where ./wish is argument #1 and batch_file.txt is argument #2 therefore the count or argc will
    // equal two then call the batch method.
    if (argc == 2) {
        return runBatchFile(argv[1]);
    }

    char *line = NULL;         // buffer that getline() will allocate/resize
    size_t cap = 0;            // capacity hint for getline()

        // --- shell search path (needed by parallel launcher) ---
    char *pathv[64] = {0};
    int   pathc = 1;
    pathv[0] = "/bin";   // default per spec


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
        if (strchr(line, '&')) {
            run_parallel_line(line, pathv, pathc);   // <- your function
            continue;
        }

    }
}

// trim leading/trailing spaces
static char* trim_ws(char *s){
    if (!s) return s;
    while (*s==' ' || *s=='\t') s++;
    if (*s=='\0') return s;
    char *e = s + strlen(s) - 1;
    while (e > s && (*e==' ' || *e=='\t' || *e=='\n' || *e=='\r')) e--;
    e[1] = '\0';
    return s;
}

// Find executable in pathv using access(X_OK)
static char *find_exec(char *cmd, char **pathv, int pathc) {
    if (pathc == 0) return NULL;
    if (strchr(cmd, '/')) return NULL; 
    for (int i = 0; i < pathc; i++) {
        size_t need = strlen(pathv[i]) + 1 + strlen(cmd) + 1;
        char *full = malloc(need);
        if (!full) return NULL;
        snprintf(full, need, "%s/%s", pathv[i], cmd);
        if (access(full, X_OK) == 0) return full; // caller frees
        free(full);
    }
    return NULL;
}



