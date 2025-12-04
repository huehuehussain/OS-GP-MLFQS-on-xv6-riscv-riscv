#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct procinfo info;
  
  printf("Testing getprocinfo() syscall...\n");
  printf("Getting info for current process (PID %d)\n", getpid());
  
  if(getprocinfo(&info) < 0) {
    printf("getprocinfo() failed!\n");
    exit(1);
  }
  
  printf("\n=== Process Information ===\n");
  printf("PID: %d\n", info.pid);
  printf("Process Name: %s\n", info.name);
  printf("State: %d\n", info.state);
  printf("Queue Level: %d\n", info.queue_level);
  printf("Time in Queue: %d ticks\n", info.time_in_queue);
  printf("Total Time Slices: %d\n", (int)info.time_slices);
  printf("=========================\n");
  
  printf("\ngetprocinfo() syscall works correctly!\n");
  
  exit(0);
}
