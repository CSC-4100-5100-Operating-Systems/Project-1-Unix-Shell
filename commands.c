// This file contains code to process Linux commands entered in the shell's interactive mode
// Written by Oluwadara Odukoya

#include <stdio.h>
#include <string.h>
#include <unistd.h>    // access, chdir, fork, execv
#include <sys/wait.h>  // waitpid
#include <stdlib.h>    // exit, strdup
#include <limits.h>    // PATH_MAX
#include <linux/limits.h>

// This functions splits the user's input into tokens
void tokenize_input(char *line, char **tokens){
    int i=0;
    char *token;

    while ((token = strsep(&line, " \t")) != NULL){
        if (*token == '\0'){
            continue; // Skip empty tokens
        }
        tokens[i] = token;
        // printf("%s\n", token);
        i++;
    }
    tokens[i] = NULL; // End list of tokens with NULL (necessary for execv
}

// Print a general error message, as required
void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void execute_command(char **tokens) {
    // The command is the first token
    char *command = tokens[0];
    if (command == NULL) {
        return; // Nothing to do
    }

    // 1. Define the search paths
    char *search_paths[64] = {"/bin", "/usr/bin", NULL}; // NULL-terminated
    int search_paths_count   = 2;

    // count args once (tokens is NULL-terminated)
    int argc = 0;
    while (tokens[argc] != NULL) argc++;

    // **************** built-ins: exit, cd, path ****************
    // exit: no arguments allowed
    if (strcmp(command, "exit") == 0) {
        if (argc != 1) {
            print_error();
            return;
        }
        exit(0);
    }

    // cd: exactly 1 argument
    if (strcmp(command, "cd") == 0) {
        if (argc != 2) {
            print_error();
            return;
        }
        if (chdir(tokens[1]) != 0) {
            print_error(); // or perror("cd");
        }
        return; // handled in parent
    }

    // path: 0 or more args; overwrites search path
    if (strcmp(command, "path") == 0) {
        // clear existing list, freeing only heap strings
        for (int i = 0; i < search_paths_count; i++) {
            if (search_paths[i] &&
                search_paths[i] != (char*)"/bin" &&
                search_paths[i] != (char*)"/usr/bin") {
                free(search_paths[i]);
            }
            search_paths[i] = NULL;
        }
        search_paths_count = 0;

        // add each argument as a new path entry
        // e.g., "path /bin /usr/bin"
        for (int i = 1; i < argc && i < 64; i++) {
            char *copy = strdup(tokens[i]);
            if (!copy) { print_error(); break; }
            search_paths[search_paths_count++] = copy;
        }
        // If no args given, search_paths_count stays 0 => only built-ins will work
        return;
    }
    // **************** end built-ins **********************************

    // If PATH list is empty, external commands are disabled
    if (search_paths_count == 0) {
        print_error();
        return;
    }

    char full_path[PATH_MAX];
    int found = 0;

    // 2. Loop through the paths to find the executable
    for (int i = 0; search_paths[i] != NULL; i++) {
        // Build the full path (e.g., "/bin/ls")
        snprintf(full_path, sizeof(full_path), "%s/%s", search_paths[i], command);

        // 3. Check if the file exists and is executable
        if (access(full_path, X_OK) == 0) {
            found = 1;
            break; // Exit the loop once found
        }
    }

    if (!found) {
        print_error();
        return;
    }

    // 4. Fork a new process to run the command
    pid_t pid = fork();

    if (pid < 0) { // Fork failed
        print_error();
        return;
    } else if (pid == 0) { // This is the child process
        // 5. Execute the command
        execv(full_path, tokens);
        // If execv returns, it means an error occurred
        print_error();
        exit(1);
    } else { // This is the parent process
        // 6. Wait for the child to finish
        int status;
        waitpid(pid, &status, 0);
    }
}