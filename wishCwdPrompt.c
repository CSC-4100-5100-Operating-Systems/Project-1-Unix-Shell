// wishCwdPrompt.c
#include <stdio.h>          // printf, snprintf
#include <stdlib.h>         // getenv
#include <string.h>         // strlen, strncmp
#include <unistd.h>         // getcwd, gethostname
#include <linux/limits.h>   // PATH_MAX
#include "wishCwdPrompt.h"  // header file

// This file gets the cwd(current working directory) and adds it 
// to the "wish> " prompt so user can see their current directory location
void make_prompt(char *out, size_t cap) {
    char host[256] = "?";
    gethostname(host, sizeof(host));

    const char *user = getenv("USER");
    if (!user) user = "?";

    char cwd[PATH_MAX] = "?";
    (void)getcwd(cwd, sizeof(cwd));

    const char *home = getenv("HOME");
    const char *shown = cwd;
    char display[PATH_MAX];

    if (home && strncmp(cwd, home, strlen(home)) == 0) {
        snprintf(display, sizeof(display), "~%s", cwd + strlen(home));
        shown = display;
    }

    snprintf(out, cap, "%s@%s:%s$ wish> ", user, host, shown);
}
