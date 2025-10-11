// Batch work for the project handled by Brian Kemp

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>     // opendir, readdir, closedir
#include <string.h>
#include "batch.h"
#include <unistd.h>     // execv
#include <limits.h>     // PATH_MAX
#include <linux/limits.h>
#include <sys/wait.h>   // waitpid()  

static char *directories[64] = { "/bin", "/usr/bin" };
static int directoryCount = 2;

// Reference: https://www.w3schools.com/c/ref_string_strtok.php
// How to split a string
// char myStr[] = "Learn C++ at W3schools";
// char * myPtr = strtok(myStr, " ");
// while(myPtr != NULL) {
//   cout << myPtr << "\n";
//   myPtr = strtok(NULL, " ");
// }

int runBatchFile(const char *filepath) {
    FILE *in = fopen(filepath, "r");
    if (!in) { perror("fopen"); return 1; }

    char *line = NULL;
    unsigned long cap = 0;
    long n;

    while ((n = getline(&line, &cap, in)) != -1) {
        // tokenize the line into lineArray[] using strsep
        char *rest = line;
        char *lineArray[16];
        int index = 0;
        char *strsepToken;

        // strsep reference: https://c-for-dummies.com/blog/?p=1769
        while ((strsepToken = strsep(&rest, " \t\n")) != NULL) {
            if (*strsepToken == '\0') continue;          // skip empty tokens
            if (index == 15) break;          // leave room for NULL
            lineArray[index++] = strsepToken;
        }
        lineArray[index] = NULL;
        if (index == 0) continue; // if there is a blank line, then skip to next.

        // not sure why you would have exist in a batch file but if it exists this should handle it.
        if (strcmp(lineArray[0], "exit") == 0) {
            // check and make sure exit has no arguments coming with it. 
            if (index != 1) {
                fprintf(stderr, "exit: takes no arguments\n");
                continue;
            }
            free(line); // freeing files text
            fclose(in); // closing file
            exit(0); // executing exit command. 
        }

        // this will handle cd command.
        if (strcmp(lineArray[0], "cd") == 0) {
            // check to make sure cd command only has one set of arguments coming with it.
            if (index != 2) {
                fprintf(stderr, "cd: expected 1 argument\n");
                continue;
            }
            // int chdir(const char *__path) returns a number for complete so if it returns 0 that makes it was able to complete. 
            // if it is not able to complete then throw an error. 
            if (chdir(lineArray[1]) != 0) {
                perror("cd");
            }
            continue;
        }

        // this handles the PATH command. 
        if (strcmp(lineArray[0], "path") == 0) {
            // first we clear out any exist paths/directories except the /bin and /usr/bin locations.
            for (int i = 0; i < directoryCount; i++) {
                if (directories[i] &&
                    directories[i] != (char*)"/bin" &&
                    directories[i] != (char*)"/usr/bin") {
                    free(directories[i]); // free only heap strings
                }
                directories[i] = NULL;
            }

            // overwrite the old path completely. if the user provides
            // no args, directoryCount stays 0 → only built-ins will work.
            directoryCount = 0;

            // add each directory argument after "path" to the new search path.
            // e.g., "path /bin /usr/bin" → those two become the entire path.
            for (int i = 1; i < index && i < 64; i++) {
                directories[directoryCount++] = strdup(lineArray[i]); // persist beyond this line
            }
            continue;
        }

        // lineArray[0] is the command (e.g., "ls")
        const char *cmd = lineArray[0];

        // find which directory contains the command
        const char *dir = findCommandDirectory(cmd);
        if (!dir) {
            fprintf(stderr, "command not found: %s\n", cmd);
            continue;
        }

        // build full path: /bin/ls or /usr/bin/ls
        char full[PATH_MAX];
        snprintf(full, sizeof full, "%s/%s", dir, cmd);

        // run it
        int rc = fork();
        if (rc < 0) {
            perror("fork");
            continue;
        }
        // took this from the lession on forks()
        if (rc == 0) {
            // child: run execv() and run the incoming command.
            execv(full, lineArray);
            perror("execv");
            _exit(127);
        } else {
            // parent: wait for this child to finish then continue on with the rest of the file if any. 
            int status;
            while (waitpid(rc, &status, 0) < 0){}
        }
    }

    free(line); // freeing the file text
    fclose(in); // closing the file. 
    return 0; // returning 0 to end program and close batch mode. 
}

// Reference: https://www.w3schools.com/c/c_arrays_size.php
// int myNumbers[] = {10, 25, 50, 75, 100};
// int length = sizeof(myNumbers) / sizeof(myNumbers[0]);
// printf("%d", length);  // Prints 5

const char *findCommandDirectory(const char *command) {
    for (int i = 0; i < directoryCount; i++) {
        DIR *directory = opendir(directories[i]);
        if (!directory) continue;

        struct dirent *entry;
        while ((entry = readdir(directory)) != NULL) {
            if (strcmp(entry->d_name, command) == 0) {
                closedir(directory);
                return directories[i];
            }
        }
        closedir(directory);
    }
    return NULL;
}