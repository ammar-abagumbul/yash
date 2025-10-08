#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define PROMPT "## 3230yash >> "
#define MAX_CMDS 30
#define MAX_TOKEN_LEN 1025
#define EXIT_CMD "exit"

enum {
  V_EXIT_REQUEST = 0, // valid exit request
  E_EXIT_REQUEST,     // invalid exit request
  NO_EXIT_REQUEST     // no request
};

typedef struct {
  int pipe_count;
  char **pipes;
  pid_t pids[MAX_CMDS];
} Command;

void handle_SIGINT(int sig);
void free_tokens(char **tokens, int num_commands);
int is_space(char ch);
int validate_cmd(char *cmd);
void tokenize(Command *cmd, char *buffer);
char **parse_args(const char *buffer);
int readline(char *buffer, size_t size);
void print_tokens(char *tokens);
void init_commands(Command *cmd, char* buffer);

#endif /* SHELL_H */
