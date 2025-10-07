/**
 * @file wish.c
 * @brief Implementation of the base Unix-like shell named "wish".
 *
 * This file contains the core functionality for a minimal Unix shell called "wish".
 * The shell supports interactive and batch modes, built-in commands (exit, cd, path),
 * execution of external commands by searching in specified paths, and output redirection.
 * It does not support parallel commands in this base implementation (assumed to be handled separately).
 *
 * Key features:
 * - Interactive mode: Displays a prompt including the current working directory, reads user input.
 * - Batch mode: Reads commands from a file, no prompt.
 * - Built-in commands:
 *   - exit: Exits the shell (no arguments allowed).
 *   - cd: Changes the current directory (one argument required).
 *   - path: Sets the search paths for external commands (overwrites existing paths).
 * - External commands: Forks a child process, searches paths, executes with execv, waits for completion.
 * - Redirection: Supports '>' for output redirection to a file (truncates if exists).
 * - Error handling: Prints "An error has occurred" to stderr for any errors and continues (or exits in fatal cases).
 *
 * Compilation: gcc -o wish wish.c
 * Usage:
 *   Interactive: ./wish
 *   Batch: ./wish batchfile.txt
 *
 * Assumptions:
 * - No support for parallel commands (& operator) in this base version.
 * - No quoting or escaping in command lines.
 * - Tabs and multiple spaces are treated as single delimiters.
 * - Initial search path: /bin
 * - Prompt format: "wish (cwd)> " where cwd is the current working directory.
 *
 * @author Lewis Forrest Bates lfbates42@tntech.edu
 * @author Oluwadara Odukoya ojodukoya42@tntech.edu
 * @author Meicheng Xiao mxiao42@tntech.edu
 * @author Brian Kemp bkemp42@tntech.edu
 * @date October 6th, 2025
 */

 #define _GNU_SOURCE  // For getline, asprintf, etc.
 #include <stdio.h>   // Standard I/O functions
 #include <stdlib.h>  // Memory allocation, exit
 #include <string.h>  // String manipulation
 #include <unistd.h>  // fork, execv, chdir, getcwd, access
 #include <sys/wait.h> // waitpid
 #include <sys/types.h> // pid_t
 #include <sys/stat.h>  // For stat/mode in open
 #include <fcntl.h>     // open, O_WRONLY, etc.
 
 /**
  * @brief Global array of search paths for external commands.
  *
  * This is a NULL-terminated array of strings representing directories to search for executables.
  * Initially set to {"/bin", NULL}.
  */
 char **g_paths = NULL;
 
 /**
  * @brief Number of paths in g_paths (excluding NULL terminator).
  */
 int g_num_paths = 0;
 
 /**
  * @brief Prints the standard error message to stderr.
  *
  * This function is used for all error reporting as per project specifications.
  * It does not exit the program unless called in a fatal context.
  */
 void print_error() {
     fprintf(stderr, "An error has occurred\n");
 }
 
 /**
  * @brief Frees the global paths array.
  *
  * Releases memory allocated for each path string and the array itself.
  * Resets g_paths to NULL and g_num_paths to 0.
  */
 void free_paths() {
     if (g_paths) {
         for (int i = 0; i < g_num_paths; i++) {
             free(g_paths[i]);
         }
         free(g_paths);
         g_paths = NULL;
         g_num_paths = 0;
     }
 }
 
 /**
  * @brief Initializes the global paths to default (/bin).
  *
  * Allocates and sets the initial search path.
  *
  * @return 0 on success, -1 on allocation failure.
  */
 int init_paths() {
     free_paths();
     g_paths = malloc(2 * sizeof(char*));
     if (!g_paths) return -1;
     g_paths[0] = strdup("/bin");
     if (!g_paths[0]) {
         free(g_paths);
         return -1;
     }
     g_paths[1] = NULL;
     g_num_paths = 1;
     return 0;
 }
 
 /**
  * @brief Tokenizes a command line into an array of tokens.
  *
  * Splits the input line by spaces and tabs, ignoring multiple consecutive delimiters.
  * Allocates a NULL-terminated array of duplicated strings.
  *
  * @param line The input command line (will be modified by strsep).
  * @param num_tokens Output parameter for the number of tokens (excluding NULL).
  * @return NULL-terminated array of tokens, or NULL on failure.
  */
 char **tokenize(char *line, int *num_tokens) {
     char **tokens = NULL;
     int capacity = 0;
     *num_tokens = 0;
     char *delims = " \t";
     char *tok;
 
     while ((tok = strsep(&line, delims))) {
         if (*tok == '\0') continue;  // Skip empty tokens from multiple spaces
 
         if (*num_tokens >= capacity) {
             capacity = capacity ? capacity * 2 : 8;
             tokens = realloc(tokens, capacity * sizeof(char*));
             if (!tokens) return NULL;
         }
         tokens[*num_tokens] = strdup(tok);
         if (!tokens[*num_tokens]) return NULL;
         (*num_tokens)++;
     }
 
     // Add NULL terminator
     tokens = realloc(tokens, (*num_tokens + 1) * sizeof(char*));
     if (!tokens) return NULL;
     tokens[*num_tokens] = NULL;
     return tokens;
 }
 
 /**
  * @brief Frees a token array.
  *
  * Releases each string and the array itself.
  *
  * @param tokens The NULL-terminated token array.
  */
 void free_tokens(char **tokens) {
     if (tokens) {
         for (int i = 0; tokens[i]; i++) {
             free(tokens[i]);
         }
         free(tokens);
     }
 }
 
 /**
  * @brief Handles built-in commands.
  *
  * Checks if the command is a built-in (exit, cd, path) and executes it.
  * Prints error message on failure but continues (except for exit).
  *
  * @param tokens NULL-terminated array of tokens (cmd + args).
  * @return 1 if handled as built-in, 0 otherwise.
  */
 int handle_builtin(char **tokens) {
     if (!tokens[0]) return 0;
 
     if (strcmp(tokens[0], "exit") == 0) {
         if (tokens[1]) {
             print_error();
             return 1;
         }
         free_tokens(tokens);
         free_paths();
         exit(0);
     } else if (strcmp(tokens[0], "cd") == 0) {
         int arg_count = 0;
         while (tokens[++arg_count]);  // Count args after cmd
         if (arg_count != 1) {
             print_error();
             return 1;
         }
         if (chdir(tokens[1]) < 0) {
             print_error();
         }
         return 1;
     } else if (strcmp(tokens[0], "path") == 0) {
         free_paths();
         int arg_count = 0;
         while (tokens[++arg_count]);  // Count args
         g_num_paths = arg_count;
         if (g_num_paths > 0) {
             g_paths = malloc((g_num_paths + 1) * sizeof(char*));
             if (!g_paths) {
                 print_error();
                 return 1;
             }
             for (int i = 0; i < g_num_paths; i++) {
                 g_paths[i] = strdup(tokens[i + 1]);
                 if (!g_paths[i]) {
                     print_error();
                     free_paths();
                     return 1;
                 }
             }
             g_paths[g_num_paths] = NULL;
         } else {
             g_paths = malloc(sizeof(char*));
             if (g_paths) g_paths[0] = NULL;
         }
         return 1;
     }
     return 0;
 }
 
 /**
  * @brief Executes an external command with optional redirection.
  *
  * Forks a child process, handles redirection if specified, searches paths for the executable,
  * executes with execv, and waits in parent.
  *
  * @param tokens NULL-terminated array of tokens (cmd + args + possibly > file).
  */
 void execute_external(char **tokens) {
     int nt = 0;
     while (tokens[nt]) nt++;
 
     // Find redirection
     int redir_idx = -1;
     char *redir_file = NULL;
     for (int i = 0; i < nt; i++) {
         if (strcmp(tokens[i], ">") == 0) {
             if (redir_idx != -1 || i != nt - 2) {  // Multiple > or not at end
                 print_error();
                 return;
             }
             redir_idx = i;
             redir_file = tokens[i + 1];
         }
     }
 
     if (redir_idx >= 0) {
         nt = redir_idx;  // Truncate tokens at >
         tokens[nt] = NULL;
     }
 
     if (!tokens[0]) return;  // Empty command
 
     if (g_num_paths == 0) {
         print_error();
         return;
     }
 
     // Find executable in paths
     char *full_path = NULL;
     for (int i = 0; i < g_num_paths; i++) {
         if (asprintf(&full_path, "%s/%s", g_paths[i], tokens[0]) < 0) {
             print_error();
             return;
         }
         if (access(full_path, X_OK) == 0) {
             break;
         }
         free(full_path);
         full_path = NULL;
     }
 
     if (!full_path) {
         print_error();
         return;
     }
 
     // Handle redirection
     int fd = -1;
     if (redir_file) {
         fd = open(redir_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
         if (fd < 0) {
             print_error();
             free(full_path);
             return;
         }
     }
 
     pid_t pid = fork();
     if (pid < 0) {
         print_error();
         if (fd >= 0) close(fd);
         free(full_path);
         return;
     } else if (pid == 0) {  // Child
         if (fd >= 0) {
             if (dup2(fd, STDOUT_FILENO) < 0) {
                 print_error();
                 close(fd);
                 free(full_path);
                 exit(1);
             }
             close(fd);
         }
         execv(full_path, tokens);
         print_error();  // If execv fails
         free(full_path);
         exit(1);
     } else {  // Parent
         int status;
         waitpid(pid, &status, 0);
     }
 
     if (fd >= 0) close(fd);
     free(full_path);
 }
 
 /**
  * @brief Main entry point for the wish shell.
  *
  * Initializes paths, handles command-line arguments for batch mode,
  * enters the main loop to read and process commands.
  *
  * @param argc Number of command-line arguments.
  * @param argv Array of command-line arguments.
  * @return 0 on normal exit, 1 on error.
  */
 int main(int argc, char *argv[]) {
     if (init_paths() < 0) {
         print_error();
         return 1;
     }
 
     FILE *input = stdin;
     int is_interactive = 1;
 
     if (argc > 2) {
         print_error();
         free_paths();
         return 1;
     } else if (argc == 2) {
         input = fopen(argv[1], "r");
         if (!input) {
             print_error();
             free_paths();
             return 1;
         }
         is_interactive = 0;
     }
 
     char *line = NULL;
     size_t cap = 0;
 
     while (1) {
         if (is_interactive) {
             char cwd[1024];
             if (getcwd(cwd, sizeof(cwd)) == NULL) {
                 strcpy(cwd, "?");
             }
             printf("wish (%s)> ", cwd);
             fflush(stdout);
         }
 
         ssize_t n = getline(&line, &cap, input);
         if (n == -1) {  // EOF
             break;
         }
 
         // Strip trailing newline
         if (n > 0 && line[n - 1] == '\n') line[n - 1] = '\0';
 
         // Ignore empty lines
         if (strlen(line) == 0) continue;
 
         // Tokenize
         int num_tokens;
         char **tokens = tokenize(line, &num_tokens);
         if (!tokens) {
             print_error();
             continue;
         }
 
         // Handle built-in
         if (handle_builtin(tokens)) {
             free_tokens(tokens);
             continue;
         }
 
         // Execute external
         execute_external(tokens);
 
         free_tokens(tokens);
     }
 
     free(line);
     if (input != stdin) fclose(input);
     free_paths();
     return 0;
 }