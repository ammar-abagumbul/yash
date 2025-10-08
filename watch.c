
#include "yash.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#define WATCH_CMD "watch"
#define INTERVAL 500000000
#define LINE_WIDTH 50

int main(int argc, char * argv[]) {

  if (strcmp(argv[0], WATCH_CMD) != 0) {
    perror("Invoked with incorrect command. Terminating");
    exit(EXIT_FAILURE);
  }

  char summary[LINE_WIDTH * 50 + 1];
  char *pSummary = summary;

  memset(summary, '\0', sizeof(summary));

  struct timespec interval = { 0, INTERVAL};

  // create a child and run command
  char stat_path[256];
  pid_t pid = fork();
  FILE *fp;

  if (pid == 0) {
    if (execvp(argv[1], argv + 1) == -1) {        // Shift by one to escape watch command
      perror("Failed to execute command");
      return -1;
    }
  } else if (pid > 0)  {
    int status;
    pid_t c_pid = waitpid(pid, &status, WNOHANG);

    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);

    while (1) {
      if (c_pid == 0) {
        fp = fopen(stat_path, "r");
        if (fp == NULL) {
          perror("Failed to read proc_pid_stat file");
          //TODO: deliberate what would be a better fail safe
        }

                              // pos desc
        char state;           //3
        unsigned long minflt; //10
        unsigned long majflt; //12
        unsigned long utime;  //14 time spent in user mode (t / sysconf(_SC_CLK_TCK))
        unsigned long stime;  //15 time spent in kern mode (t / sysconf(_SC_CLK_TCK))
        unsigned long vsize;  //23 Virtual memory size
        int processor;        //39

        fscanf(fp, "%*s %*s %c %*s %*s %*s %*s %*s %*s %lu %*s %lu %*s %lu %lu %*s %*s %*s %*s %*s %*s %*s %lu %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %d %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s ",
          &state, &minflt, &majflt, &utime, &stime, &vsize, &processor
        );

        char temp_buff[50];
        snprintf(temp_buff, LINE_WIDTH, "");

        long clk_tck = sysconf(_SC_CLK_TCK);
        double usr_sec = (double) utime / clk_tck;
        double sys_sec = (double) stime / clk_tck;

        int formatted_len = snprintf(pSummary, LINE_WIDTH,
          "%-5c %-5d %-5lu %-5lu %-9.2f %-6.2f %-6lu \n",
          state, processor, minflt, majflt, usr_sec, sys_sec, vsize);

        if (formatted_len != LINE_WIDTH) {
          perror("[WARN] Formatted summary line does not match line width");
        }

        pSummary += formatted_len;

        nanosleep(&interval, NULL);

      } else if (c_pid > 0) {
        printf("%s", summary);
      } else {
        perror("Fatal error: waitpid failed");
        exit(EXIT_FAILURE);
      }
    }
  } else {
    perror("Fatal error: failed to fork");
    return -1;
  }
}
