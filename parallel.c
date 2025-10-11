#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "parallel.h"

// the spec's single error message 
static const char error_msg[] = "An error has occurred\n";

// prints the error message
static void print_error(void) {
    (void)write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
}

// trims leading/trailing whitespace in-place and returns the same pointer 
static char *trim(char *s) {
    if (!s) return s;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '\0') return s;
    char *e = s + strlen(s) - 1;
    while (e > s && (*e == ' ' || *e == '\t' || *e == '\n' || *e == '\r')) e--;
    e[1] = '\0';
    return s;
}

//splits by spaces/tabs into argv[]; returns argc
static int split_words(char *s, char **argv, int max_argv) {
    int argc = 0;
    while (s && *s) {
        char *tok = strsep(&s, " \t");
        if (!tok) break;
        tok = trim(tok);
        if (*tok == '\0') continue;
        if (argc >= max_argv - 1) break;
        argv[argc++] = tok;
    }
    argv[argc] = NULL;
    return argc;
}

//parses "cmd args > file" into argv[] and out_file
static int parse_job(char *job, char **argv, int max_argv, char **out_file) {
    *out_file = NULL;

  
    char *redir = strchr(job, '>');
    if (redir) {
        if (strchr(redir + 1, '>')) {
            print_error();
            return -1;
        }
        *redir = '\0';
        char *rhs = trim(redir + 1);
        char *one[4] = {0};
        int n = split_words(rhs, one, 4);
        if (n != 1) {
            print_error();
            return -1;
        }
        *out_file = one[0];
    }

    //split the left side into argv 
    char *lhs = trim(job);
    int argc = split_words(lhs, argv, max_argv);
    if (argc == 0) {
        print_error();
        return -1;
    }
    return argc;
}

// looks up an executable in search_paths; returns malloc'd full path or NULL 
static char *find_executable(const char *cmd, char **search_paths, int num_paths) {
    if (num_paths == 0 || strchr(cmd, '/')) return NULL;
    for (int i = 0; i < num_paths; i++) {
        size_t need = strlen(search_paths[i]) + 1 + strlen(cmd) + 1;
        char *full = (char *)malloc(need);
        if (!full) return NULL;
        snprintf(full, need, "%s/%s", search_paths[i], cmd);
        if (access(full, X_OK) == 0) return full;
        free(full);
    }
    return NULL;
}

// returns non-zero if '&' appears in the line
int is_parallel_line(const char *line) {
    return line && strchr(line, '&') != NULL;
}

// runs all jobs on a line in parallel and waits for them
void run_parallel_line(char *line, char **search_paths, int num_paths) {
    char *cursor = line;
    pid_t pids[256];
    int pid_count = 0;

    while (cursor && *cursor) {
        // split next job by '&'
        char *job = strsep(&cursor, "&");
        job = trim(job);
        if (*job == '\0') {
            print_error();
            return;
        }

        //parse argv and optional redirection file
        char *argv[128] = {0};
        char *out_file = NULL;
        int argc = parse_job(job, argv, 128, &out_file);
        if (argc < 0) return;

        //reject built ins here
        if (!strcmp(argv[0], "exit") || !strcmp(argv[0], "cd") || !strcmp(argv[0], "path")) {
            print_error();
            return;
        }

        //resolve the executable path
        char *prog = find_executable(argv[0], search_paths, num_paths);
        if (!prog) {
            print_error();
            continue;
        }

        //fork a child to run this job 
        pid_t pid = fork();
        if (pid < 0) {
            print_error();
            free(prog);
            continue;
        }

        if (pid == 0) {
            //set up optional redirection for stdout and stderr, then exec 
            if (out_file) {
                int fd = open(out_file, O_CREAT | O_TRUNC | O_WRONLY, 0666);
                if (fd < 0 || dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0) {
                    print_error();
                    if (fd >= 0) close(fd);
                    _exit(1);
                }
                close(fd);
            }
            execv(prog, argv);
            print_error();
            _exit(1);
        }

        
        free(prog);
        if (pid_count < (int)(sizeof(pids) / sizeof(pids[0]))) {
            pids[pid_count++] = pid;
        }
    }

    // wait for every child started on this line 
    for (int i = 0; i < pid_count; i++) {
        while (waitpid(pids[i], NULL, 0) == -1 && errno == EINTR) {
        }
    }
}
