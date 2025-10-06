#include "yash.h"

void handle_SIGINT(int sig) {
  printf("\n");
  signal(SIGINT, handle_SIGINT);
}
