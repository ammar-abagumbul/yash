#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

const char *EXIT_CMD = "exit";


// Signal handler
void handle_keyboard_interrupt(int sig) {
  printf("\n##3030yash >> ");
  fflush(stdout);
}

// parser
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

    if (*p2 == '|') count++;

    if (p1 != NULL && *p1 == *p2 && *p1 == '|') {
      printf("3030yash: Should not have two consequetive | without in-between command\n");
      return -1;
    }
  }

  if (*p1 == '|') {
    printf("3030yash: Invalid pipe sequence\n");
    return -1;
  }

  return count + 1;
}

void execute(char *cmd, int count) {
  char *saveptr;
  char *part = strtok_r(cmd, "|", &saveptr);
  int tracker = 1;
  int oldfd[2];
  while (part != NULL) {

    char *args[1025];
    char *token = strtok(part, "\n");
    token = strtok(token, " ");
    int i = 0;

    while (token != NULL) {
      args[i] = token;
      size_t len = strlen(token);

      if(token[len - 1] == '\n') {
        args[i][len - 1] = '\0';
      }

      token = strtok(NULL, " ");
      i++;
    }

    args[i] = NULL;

    int fd[2];
    if (pipe(fd) == -1) {
      perror("fatal error while creating pipe, aborting");
      abort();
    }

    pid_t pid = fork();

    if (pid < 0) {
      perror("fatal error while forking, aborting");
      abort();
    } else if (pid == 0) {

        if (tracker == 1 && count > 1) {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
        } else if (tracker == count) {
            dup2(oldfd[0], STDIN_FILENO);
            close(oldfd[1]);
            close(fd[0]);
            close(fd[1]);
        } else {
            dup2(fd[1], STDOUT_FILENO);
            dup2(oldfd[0], STDIN_FILENO);
            close(oldfd[1]);
            close(fd[0]);
            close(fd[1]);
        }

        if (execvp(args[0], args) == -1) {
            printf("%s: No such file or directory\n", args[0]);
            abort();
        }
    } else {
        if (tracker > 1) {
            close(oldfd[0]);
            close(oldfd[1]);
        }
        oldfd[0] = fd[0];
        oldfd[1] = fd[1];
        tracker++;
    }
    part = strtok_r(NULL, "|", &saveptr);
  }
  close(oldfd[0]);
  close(oldfd[1]);
  for (int i = 0; i < count; i++) {
    wait(NULL);
  }
}

int main(int argc, char* argv[])
{

  pid_t pid = getpid();
  printf("Process pid: %d: \n", (int) pid);
  char cmd[1025];

  if (signal(SIGINT, handle_keyboard_interrupt) == SIG_ERR)
  {
    perror("fatal error while setting signal handler, aborting");
    abort();
  }

  do {
    printf("##3030yash >> ");
    fgets(cmd, sizeof(cmd), stdin);

    if (strncmp(cmd, EXIT_CMD, 4) == 0) {
      exit(EXIT_SUCCESS);
    }

    int count = validate_cmd(cmd);

    if (count == -1) {
      continue;
    }

    execute(cmd, count);

  } while (1);
}

