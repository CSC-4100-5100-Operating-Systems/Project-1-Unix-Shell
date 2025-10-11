// wish.c — minimal starter shell
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"
#include "batch.h"
#include "parallel.h"
#include "wishCwdPrompt.h"
#include "linenoise.h"
#include <linux/limits.h>
#include <dirent.h>

// Found a line completion tool that will complete lines in the prompt
// e.g. cd Des + tab will become cd Desktop/
// Reference: https://github.com/antirez/linenoise
static void file_completion(const char *buf, linenoiseCompletions *lc) {
    // find start of last token
    const char *p = buf + strlen(buf);
    while (p > buf && p[-1] != ' ' && p[-1] != '\t') p--;
    const char *prefix = p;
    size_t plen = strlen(prefix);

    DIR *d = opendir(".");
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (strncmp(e->d_name, prefix, plen) == 0) {
            char cand[PATH_MAX];
            // reconstruct full line: everything before prefix + candidate
            snprintf(cand, sizeof(cand), "%.*s%s",
                     (int)(p - buf), buf, e->d_name);
            linenoiseAddCompletion(lc, cand);
        }
    }
    closedir(d);
}

int main(int argc, char *argv[]) {
    FILE *in = stdin;          // read commands from standard input (keyboard)
  
    // batch mode check. if the arguments coming is at least 2, e.g. "./wish batch_file.txt
    // where ./wish is argument #1 and batch_file.txt is argument #2 therefore the count or argc will
    // equal two then call the batch method.
    if (argc == 2) {
        return runBatchFile(argv[1]);
    }

    linenoiseSetCompletionCallback(file_completion);

    char *line = NULL;         // buffer that getline() will allocate/resize
    size_t cap = 0;            // capacity hint for getline()
  
    // --- shell search path (needed by parallel launcher) ---
    char *pathv[64] = {0};
    int pathc = 2;
    pathv[0] = "/bin";
    pathv[1] = "/usr/bin";

    // main loop (run until "exit" or EOF)
    while (1) {
        char prompt[PATH_MAX + 128];
        make_prompt(prompt, sizeof(prompt));     // builds the "user@host:cwd$ wish> "

        char *rl = linenoise(prompt);            // using linenoise instead of getline
        if (!rl) exit(0);                        // EOF (Ctrl+D)

        if (rl[0] == '\0') {                     // empty line
            linenoiseFree(rl);
            continue;
        }

        // built-in: exit (fast path)
        if (strcmp(rl, "exit") == 0) {
            linenoiseFree(rl);
            exit(0);
        }

        linenoiseHistoryAdd(rl); //this adds the pressing up or down and getting previously ran commands

        // Parallel commands?
        if (strchr(rl, '&')) {
            run_parallel_line(rl, pathv, pathc);
            linenoiseHistoryAdd(rl);
            linenoiseFree(rl);
            continue;
        }

        // Tokenize & execute
        char *tokens[20];
        tokenize_input(rl, tokens);
        execute_command(tokens);

        linenoiseFree(rl);
    }
}
