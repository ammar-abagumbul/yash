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
