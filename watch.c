
#include "yash.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#define WATCH_CMD "watch"
#define INTERVAL 500000000
#define LINE_WIDTH 50

typedef struct {
  // pos desc
  char state;           //3
  unsigned long minflt; //10
  unsigned long majflt; //12
  unsigned long utime;  //14 time spent in user mode (t / sysconf(_SC_CLK_TCK))
  unsigned long stime;  //15 time spent in kern mode (t / sysconf(_SC_CLK_TCK))
  unsigned long vsize;  //23 Virtual memory size
  int processor;        //39
} ProcessInfo;

void read_pid_proc_stat(ProcessInfo *processInfo, char *stat_path) {

  FILE *fp = fopen(stat_path, "r");
  if (fp == NULL) {
    perror("Failed to read proc_pid_stat file");
    //TODO: deliberate what would be a better fail safe
  }

  fscanf(fp, "%*s %*s %c %*s %*s %*s %*s %*s %*s %lu %*s %lu %*s %lu %lu %*s %*s %*s %*s %*s %*s %*s %lu %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %d %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s ",
    &processInfo->state, &processInfo->minflt, &processInfo->majflt, &processInfo->utime, &processInfo->stime, &processInfo->vsize, &processInfo->processor
  );

  // Close the file
  if (fclose(fp) == EOF) {
      perror("Error closing file");
  }
}

int main(int argc, char * argv[]) {

  // if (strcmp(argv[0], WATCH_CMD) != 0) {
  //   perror("Invoked with incorrect command. Terminating");
  //   exit(EXIT_FAILURE);
  // }

  char summary[LINE_WIDTH * 50 + 1];
  char *pSummary = summary;

  memset(summary, '\0', sizeof(summary));

  struct timespec interval = { 0, INTERVAL};

  // create a child and run command
  char stat_path[256];
  pid_t pid = fork();
  FILE *fp;

  ProcessInfo processInfo;

  if (pid == 0) {
    if (execvp(argv[1], argv + 1) == -1) {        // Shift by one to escape watch command
      perror("Failed to execute command");
      return -1;
    }
  } else if (pid > 0)  {
    int status;
    pid_t c_pid = waitpid(pid, &status, WNOHANG);

    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);

    while (state != 'Z') {
      read_pid_proc_stat(&processInfo, stat_path);
      long clk_tck = sysconf(_SC_CLK_TCK);
      double usr_sec = (double) processInfo.utime / clk_tck;
      double sys_sec = (double) processInfo.stime / clk_tck;

      int formatted_len = snprintf(pSummary, LINE_WIDTH + 1,
        "%-5c %-5d %-5lu %-5lu %-9.2f %-6.2f %-6lu \n",
        processInfo.state, processInfo.processor, processInfo.minflt, processInfo.majflt, usr_sec, sys_sec, processInfo.vsize);

      pSummary += formatted_len;

      nanosleep(&interval, NULL);
      state = processInfo.state;
    }
    printf("STATE CPUID UTIME STIME VSIZE     MINFLT MAJFLT\n");
    printf("%s\n", summary);
  } else {
    perror("Fatal error: failed to fork");
    return -1;
  }
}
