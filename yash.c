#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>

#define PROMPT "## 3230yash >> "
#define MAX_CMDS 30
#define MAX_TOKEN_LEN 1025

typedef struct {
  int pipe_count;
  char **pipes;
  pid_t pids[MAX_CMDS];
} Command;

void handle_SIGINT(int sig) {
  printf("\n");
  signal(SIGINT, handle_SIGINT);
}

void free_tokens(char **tokens, int num_commands) {
  if (tokens == NULL || num_commands <= 0) {
    return; // sanity check
  }

  for (int i = 0; i < num_commands; i++) {
    if (tokens[i] == NULL)
      continue;      // Skip empty rows
    free(tokens[i]); // Free the row array
    tokens[i] = NULL;
  }
  free(tokens); // Free the top-level array
  tokens = NULL;
}

int is_space(char ch) {
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\v' ||
         ch == '\f';
}

int validate_cmd(char *cmd) {
  char *p1 = NULL, *p2 = cmd;
  int count = 0;
  if (*p2 == '\n') {
    return -1;
  }

  if (*p2 == '|') {
    printf("3030yash: Invalid pipe sequence\n");
    return -1;
  }

  while (*p2 != '\0') {
    if (*p2 != ' ' && *p2 != '\n') {
      p1 = p2;
    }
    p2++;

    if (*p2 == '|')
      count++;

    if (p1 != NULL && *p1 == *p2 && *p1 == '|') {
      printf("3030yash: Should not have two consequetive | without in-between "
             "command\n");
      return -1;
    }
  }

  if (*p1 == '|') {
    printf("3030yash: Invalid pipe sequence\n");
    return -1;
  }

  return count + 1;
}

void tokenize(Command *cmd, char *buffer) {
  if (cmd == NULL || buffer == NULL || *buffer == '\0' || *buffer == '|' ||
      *buffer == '\n') {
    perror("[ERROR] tokenization failed. Make sure to provide valid cmd and "
           "buffer");
    return;
  }

  cmd->pipe_count = 0;
  cmd->pipes = NULL;

  char **pipes = malloc(MAX_CMDS * sizeof(char *));
  if (!pipes) {
    return;
  }

  int pc = 0;
  pipes[pc] = malloc(MAX_TOKEN_LEN * sizeof(char));
  if (!pipes[pc]) {
    free(pipes);
    return;
  }

  int i = 0;
  int in_closure = 0;
  char clsr_char = (char)0;
  int backslash = 0;
  int in_token = 0;

  while (*buffer) {
    char ch = *buffer;

    if (backslash) {
      pipes[pc][i++] = ch;
      if (!in_closure) {
        in_token = 1;
      }
      backslash = 0;
      buffer++;
      continue;
    }

    if (ch == '\\') {
      backslash = 1;
      buffer++;
      continue;
    }

    if (ch == '|') {
      if (!in_closure) {
        pipes[pc][i++] = '\0';
        pipes[pc][i] = '\0';
        pc++;
        if (pc >= MAX_CMDS) {
          perror("[ERROR] too many pipes");
          break;
        }
        pipes[pc] = malloc(MAX_TOKEN_LEN * sizeof(char));
        if (!pipes[pc]) {
          break;
        }
        i = 0;
        in_token = 0;
      } else {
        pipes[pc][i++] = ch;
      }
      buffer++;
      continue;
    }

    if (is_space(ch)) {
      if (in_closure) {
        pipes[pc][i++] = ch;
      } else {
        if (in_token) {
          pipes[pc][i] = '\0';
          i++;
          in_token = 0;
        }
      }
      buffer++;
      continue;
    }

    if (ch == '\'' || ch == '"') {
      if (in_closure) {
        if (ch == clsr_char) {
          in_closure = 0;
        } else {
          pipes[pc][i++] = ch;
        }
      } else {
        clsr_char = ch;
        in_closure = 1;
        in_token = 1;
      }
      buffer++;
      continue;
    }

    // normal character
    pipes[pc][i++] = ch;
    if (!in_closure) {
      in_token = 1;
    }
    buffer++;
  }

  // finalize last command
  pipes[pc][i++] = '\0';
  pipes[pc][i] = '\0';

  cmd->pipe_count = pc + 1;
  cmd->pipes = pipes;
}

char **parse_args(const char *buffer) {
  if (!buffer || *buffer == '\0') {
    return NULL;
  }

  int count = 0;
  const char *p = buffer;

  // count args
  while (*p) {
    count++;
    do {
      p++;
    } while (*p != '\0' && p[-1] != '\0');
    p++;
    if (*p == '\0')
      break;
  }

  // reset p
  p = buffer;
  char **args = malloc((count + 1) * sizeof(char *));

  for (int i = 0; i < count; i++) {
    args[i] = (char *)p;
    p += strlen(p) + 1;
  }

  args[count] = NULL;
  return args;
}

int readline(char *buffer, size_t size) {
  if (fgets(buffer, size, stdin) != NULL) {
    buffer[strcspn(buffer, "\n")] = '\0';
    if (buffer[0] == '\0')
      return 0;
    return 1;
  } else {
    exit(0);
  }
}

void init_commands(Command *cmd, char* buffer) {
  // Initialize commands here
  memset(cmd->pids, 0, sizeof(cmd->pids));
  tokenize(cmd, buffer);
}

int main(int argc, char *argv[]) {
  char buffer[1025];
  pid_t master_pid = getpid();

  // create a session
  /*
  if (setsid() == -1) {
    perror("Fatal error creating session");
    exit(EXIT_FAILURE);
  }

  if (ioctl(STDIN_FILENO, TIOCSCTTY, 0) == -1) {
    perror("ioctl TIOCSCTTY failed");
    exit(EXIT_FAILURE);
  }
  */

  signal(SIGINT, handle_SIGINT);

  do {
    printf("%s", PROMPT);

    if (readline(buffer, sizeof(buffer)) == 1) {

      if (!validate_cmd(buffer))
        continue;

      Command cmd;
      init_commands(&cmd, buffer);

      // create pipes
      int pipes[cmd.pipe_count][2];
      for (int i = 0; i < cmd.pipe_count; i++) {
        if (pipe(pipes[i]) == -1) {
          perror("Fatal error creating pipes");
          exit(EXIT_FAILURE);
        }
      }

      for (int i = 0; i < cmd.pipe_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {

          if (i == 0 && cmd.pipe_count > 1) {
            dup2(pipes[i][1], STDOUT_FILENO);
          } else if (i == cmd.pipe_count - 1) {
            dup2(pipes[i][0], STDIN_FILENO);
          } else {
            dup2(pipes[i][1], STDOUT_FILENO);
            dup2(pipes[i - 1][0], STDIN_FILENO);
          }

          if (i == 0){
            setpgid(0, 0);
          } else {
            setpgid(0, cmd.pids[0]);
          }

          // restore SIGINT handler to default
          signal(SIGINT, SIG_DFL);

          // close all fds in child
          for (int k = 0; k < cmd.pipe_count; k++) {
            close(pipes[k][0]);
            close(pipes[k][1]);
          }

          char **args = parse_args(cmd.pipes[i]);
          if (execvp(args[0], args) == -1) {
            perror("Fatal error executing command");
            exit(EXIT_FAILURE);
          }
        } else if (pid > 0) {
          cmd.pids[i] = pid;
        } else if (pid < 0) {
          perror("Fatal error creating child process");
          exit(EXIT_FAILURE);
        }
      }

      // close all fds in parent
      for (int k = 0; k < cmd.pipe_count; k++) {
        close(pipes[k][0]);
        close(pipes[k][1]);
      }

      // wait for all children to finish
      for (int i = 0; i < cmd.pipe_count; i++) {
        int status;
        waitpid(cmd.pids[i], &status, 0);

        if (WIFSIGNALED(status)) {
          int sig = WTERMSIG(status);
          printf("Child %d terminated by signal %d (%s)\n", i+1, sig, strsignal(sig));

          if (WCOREDUMP(status))
            printf(" (Core dumped)\n");
        } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
          printf("Child %d exited with status %d\n", i+1, WEXITSTATUS(status));
        }
      }

      // free tokens
      free_tokens(cmd.pipes, cmd.pipe_count);
    }

    // cleanup buffer and take control of terminal
    memset(buffer, 0, sizeof(buffer));
  } while (1);

  return 0;
}

