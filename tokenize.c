#include "yash.h"

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

void init_commands(Command *cmd, char* buffer) {
  // Initialize commands here
  memset(cmd->pids, 0, sizeof(cmd->pids));
  tokenize(cmd, buffer);
}
