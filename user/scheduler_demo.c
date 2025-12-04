#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/proc.h"

#define NUM_CHILDREN 3
#define CPU_WORK 10000000

// CPU-intensive function
void
cpu_intensive_work(int iterations)
{
  volatile int sum = 0;
  for(int i = 0; i < iterations; i++) {
    sum += i * i;
  }
}

// I/O-bound function
void
io_bound_work(int iterations)
{
  for(int i = 0; i < iterations; i++) {
    // Yield to scheduler periodically
    pause(1);
  }
}

int
main(int argc, char *argv[])
{
  int pid;
  struct procinfo info;
  int type = 0;  // 0 = CPU-bound, 1 = I/O-bound
  
  if(argc > 1) {
    type = atoi(argv[1]);
  }
  
  // Fork test processes
  if(type == 0) {
    // Parent process - create mixed workload
    printf("=== MLFQ Scheduler Demonstration ===\n");
    printf("Starting test processes...\n\n");
    
    // Create CPU-bound child
    if((pid = fork()) == 0) {
      // Child 1: CPU-intensive
      printf("Child 1 (CPU-bound): Started (PID %d)\n", getpid());
      
      for(int i = 0; i < 5; i++) {
        if(getprocinfo(&info) >= 0) {
          printf("  Loop %d - Queue: %d, Time in Queue: %d, Slices: %d\n",
                 i, info.queue_level, info.time_in_queue, (int)info.time_slices);
        }
        cpu_intensive_work(CPU_WORK);
      }
      printf("Child 1 (CPU-bound): Finished\n");
      exit(0);
    }
    
    // Create I/O-bound child
    if((pid = fork()) == 0) {
      // Child 2: I/O-intensive
      printf("Child 2 (I/O-bound): Started (PID %d)\n", getpid());
      
      for(int i = 0; i < 10; i++) {
        if(getprocinfo(&info) >= 0) {
          printf("  Iter %d - Queue: %d, Time in Queue: %d, Slices: %d\n",
                 i, info.queue_level, info.time_in_queue, (int)info.time_slices);
        }
        io_bound_work(1);  // Single pause iteration
      }
      printf("Child 2 (I/O-bound): Finished\n");
      exit(0);
    }
    
    // Create mixed workload child
    if((pid = fork()) == 0) {
      // Child 3: Mixed
      printf("Child 3 (Mixed): Started (PID %d)\n", getpid());
      
      for(int i = 0; i < 7; i++) {
        if(getprocinfo(&info) >= 0) {
          printf("  Iter %d - Queue: %d, Time in Queue: %d, Slices: %d\n",
                 i, info.queue_level, info.time_in_queue, (int)info.time_slices);
        }
        if(i % 2 == 0) {
          cpu_intensive_work(CPU_WORK / 2);
        } else {
          io_bound_work(1);
        }
      }
      printf("Child 3 (Mixed): Finished\n");
      exit(0);
    }
    
    // Parent process prints info about itself
    printf("\nParent process (PID %d) monitoring...\n", getpid());
    printf("Waiting for children to complete...\n\n");
    
    // Wait for all children
    for(int i = 0; i < 3; i++) {
      wait(0);
    }
    
    printf("\n=== Test Complete ===\n");
    printf("Expected behavior:\n");
    printf("- CPU-bound child should move to lower priority queues\n");
    printf("- I/O-bound child should remain at high priority\n");
    printf("- Mixed child should oscillate in priority\n");
    
  } else {
    // Direct test mode
    printf("getprocinfo() Test - Process %d:\n", getpid());
    
    for(int i = 0; i < 10; i++) {
      if(getprocinfo(&info) >= 0) {
        printf("Sample %d: PID=%d, Queue=%d, TimeInQ=%d, Slices=%d\n",
               i, info.pid, info.queue_level, info.time_in_queue, (int)info.time_slices);
      }
      pause(1);
    }
  }
  
  exit(0);
}
