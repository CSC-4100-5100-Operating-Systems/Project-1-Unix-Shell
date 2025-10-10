// This contains code to process Linux commands entered in the shell's interactive mode

#include <stdio.h>
#include <string.h>

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