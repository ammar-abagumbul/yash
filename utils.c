#include "yash.h"

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

void print_tokens(char *tokens) {
  char *p = tokens;
  do {
    if (*p == '\0') {
      putc('\\', stdout);
      putc('0', stdout);
    } else {
      putc(*p, stdout);
    }
    p++;
  } while (*p != '\0' || p[-1] != '\0');
  putc('\\', stdout);
  putc('0', stdout);
  putc('\n', stdout);
}

int count_args(const char* p) {
  int count = 0;
  while (*p) {
    count++;
    do {
      p++;
    } while (*p != '\0' && p[-1] != '\0');
    p++;
    if (*p == '\0')
      break;
  }
  return count;
}

void register_for_report(const pid_t* pids, int count) {
  for (int i = 0; i < count; i++) {
    int status;
    waitpid(pids[i], &status, 0);

    if (WIFSIGNALED(status)) {
      int sig = WTERMSIG(status);
      printf("Child %d terminated by signal %d (%s)\n", i+1, sig, strsignal(sig));

      if (WCOREDUMP(status))
        printf(" (Core dumped)\n");
    } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0 && WEXITSTATUS(status) != 127) {
      printf("Child %d exited with status %d\n", i+1, WEXITSTATUS(status));
    }
  }
}
