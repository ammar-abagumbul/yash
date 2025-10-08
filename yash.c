#include "yash.h"
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

//TODO: standardize error printing: use fprintf(stderr, "Error message\n");

int valid_exit_request(const Command *cmd) {
  if (strcmp(EXIT_CMD, cmd->pipes[0]) == 0) {
    if (count_args(cmd->pipes[0]) == 1) {
      return V_EXIT_REQUEST;
    }
    printf("Invalid use of exit, exit does not accept arguments.\n");
    return E_EXIT_REQUEST;
  }
  return NO_EXIT_REQUEST;
}

int main(int argc, char *argv[]) {
  char buffer[1025];
  pid_t master_pid = getpid();

  signal(SIGINT, handle_SIGINT);

  do {
    printf("%s", PROMPT);

    if (readline(buffer, sizeof(buffer)) == 1) {

      Command cmd;
      init_commands(&cmd, buffer);

      // check for exit
      int req = valid_exit_request(&cmd);
      if (req == V_EXIT_REQUEST) {
        //TODO: check for any resources to be released
        exit(EXIT_SUCCESS);
      } else if (req == E_EXIT_REQUEST) {
        continue;
      }

      // check for watch
      if (strcmp(cmd.pipes[0], "watch") == 0) {
        //TODO: implement watch command
        if (cmd.pipe_count > 1) {
          fprintf(stderr, "watch command does not support piping\n");
          continue;
        }
        char **args = parse_args(cmd.pipes[0]);
        pid_t pid = fork();
        if (pid == 0) {
          execvp("./watch", args);
          perror("error executing watch");
          exit(EXIT_FAILURE);
        } else if (pid > 0) {
          register_for_report(&pid, 1);
        } else {
          perror("fork");
          exit(EXIT_FAILURE);
        }
        continue;
      }

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
            dup2(pipes[i - 1][0], STDIN_FILENO);
          } else {
            dup2(pipes[i][1], STDOUT_FILENO);
            dup2(pipes[i - 1][0], STDIN_FILENO);
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

