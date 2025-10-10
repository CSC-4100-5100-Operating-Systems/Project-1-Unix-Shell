// This contains code to process Linux commands entered in the shell's interactive mode
// Written by Oluwadara Odukoya

#include <stdio.h>
#include <string.h>
#include <unistd.h>   // For access, fork, execv
#include <sys/wait.h> // For waitpid
#include <stdlib.h> // for exit()

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
    char *search_paths[] = {"/bin", "/usr/bin", NULL}; // NULL-terminated

    char full_path[1024];
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