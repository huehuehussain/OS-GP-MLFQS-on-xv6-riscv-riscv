#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Week 3: Comprehensive MLFQ Testing Program
// Tests demotion, priority boost, and fairness

struct procinfo info;

void
print_process_info(char *label)
{
  getprocinfo(&info);
  printf("%s: PID=%d, Queue=%d, Time_In_Queue=%d, Total_Slices=%lu\n",
         label, info.pid, info.queue_level, info.time_in_queue, info.time_slices);
}

void
cpu_intensive(int duration)
{
  // CPU-intensive process for testing demotion
  volatile int i, j;
  int start_queue = -1;
  int prev_queue = -1;
  
  printf("CPU-intensive process started (PID %d)\n", getpid());
  print_process_info("Initial state");
  start_queue = info.queue_level;
  
  for(int iter = 0; iter < duration; iter++) {
    // Busy loop
    for(i = 0; i < 1000000; i++) {
      j = i * 2;
    }
    
    getprocinfo(&info);
    if(info.queue_level != prev_queue) {
      printf("  Iteration %d: Demoted from queue %d to %d\n", 
             iter, prev_queue == -1 ? start_queue : prev_queue, info.queue_level);
      prev_queue = info.queue_level;
    }
  }
  
  printf("CPU-intensive process completed\n");
  print_process_info("Final state");
  exit(0);
}

void
io_intensive(int duration)
{
  // I/O-intensive process (sleeps often)
  printf("I/O-intensive process started (PID %d)\n", getpid());
  print_process_info("Initial state");
  
  int start_queue = info.queue_level;
  int demoted = 0;
  
  for(int i = 0; i < duration; i++) {
    getprocinfo(&info);
    if(info.queue_level != start_queue && !demoted) {
      printf("  WARNING: I/O process demoted to queue %d (should stay at %d)\n",
             info.queue_level, start_queue);
      demoted = 1;
    }
    
    // Short computation followed by I/O (implicit delay)
    for(int j = 0; j < 10000; j++) {
      volatile int x = j * 2;
    }
    sleep(1);  // Yield to simulate I/O
  }
  
  printf("I/O-intensive process completed\n");
  print_process_info("Final state");
  if(!demoted) {
    printf("  SUCCESS: I/O process remained at queue %d\n", info.queue_level);
  }
  exit(0);
}

void
test_cpu_demotion(void)
{
  printf("\n=== Test 1: CPU-Bound Process Demotion ===\n");
  int pid = fork();
  
  if(pid == 0) {
    // Child: CPU-intensive work
    cpu_intensive(5);
  } else {
    // Parent: wait for child
    int status;
    wait(&status);
    printf("CPU demotion test completed\n");
  }
}

void
test_io_fairness(void)
{
  printf("\n=== Test 2: I/O-Bound Process Fairness ===\n");
  int pid = fork();
  
  if(pid == 0) {
    // Child: I/O-intensive work
    io_intensive(5);
  } else {
    // Parent: wait for child
    int status;
    wait(&status);
    printf("I/O fairness test completed\n");
  }
}

void
test_mixed_workload(void)
{
  printf("\n=== Test 3: Mixed Workload Fairness ===\n");
  
  // Create multiple child processes
  int cpid1 = fork();
  if(cpid1 == 0) {
    // Child 1: CPU-intensive
    printf("Child 1 (CPU-bound) starting\n");
    cpu_intensive(3);
  }
  
  int cpid2 = fork();
  if(cpid2 == 0) {
    // Child 2: I/O-intensive
    printf("Child 2 (I/O-bound) starting\n");
    io_intensive(3);
  }
  
  // Parent waits for both
  wait(0);
  wait(0);
  printf("Mixed workload test completed\n");
}

void
test_priority_boost(void)
{
  printf("\n=== Test 4: Priority Boost Test ===\n");
  printf("Starting a CPU-bound process that should demote, then boost\n");
  
  int pid = fork();
  if(pid == 0) {
    // Track queue level changes
    printf("Process %d: Starting CPU-intensive work\n", getpid());
    print_process_info("Initial");
    
    int prev_level = info.queue_level;
    
    // Run long enough to trigger demotion
    volatile int i, j;
    for(int iter = 0; iter < 10; iter++) {
      for(i = 0; i < 1000000; i++) {
        j = i * 2;
      }
      
      getprocinfo(&info);
      if(info.queue_level != prev_level) {
        printf("  Iter %d: Queue changed from %d to %d\n", 
               iter, prev_level, info.queue_level);
        prev_level = info.queue_level;
      }
    }
    
    print_process_info("After CPU work");
    
    // At this point, should be demoted
    // After system runs for 100 ticks, should boost back
    printf("Waiting for boost (takes ~100 ticks)...\n");
    
    for(int iter = 0; iter < 20; iter++) {
      sleep(1);
      getprocinfo(&info);
      if(iter % 5 == 0) {
        printf("  Still waiting... queue=%d\n", info.queue_level);
      }
    }
    
    print_process_info("After boost period");
    exit(0);
  } else {
    // Parent waits
    int status;
    wait(&status);
    printf("Priority boost test completed\n");
  }
}

void
test_manual_boost(void)
{
  printf("\n=== Test 5: Manual Priority Boost (boostproc syscall) ===\n");
  
  int pid = fork();
  if(pid == 0) {
    printf("Child process %d starting\n", getpid());
    print_process_info("Before manual boost");
    
    // Demote self to level 3
    int cpid = getpid();
    boostproc(cpid);
    
    print_process_info("After self-boost");
    exit(0);
  } else {
    // Parent
    sleep(1);
    printf("Parent: Calling manual boost for child\n");
    boostproc(pid);
    
    int status;
    wait(&status);
    printf("Manual boost test completed\n");
  }
}

void
test_system_boost(void)
{
  printf("\n=== Test 6: System-Wide Priority Boost ===\n");
  printf("Testing boostproc(0) - boost all processes\n");
  
  int cpid = fork();
  if(cpid == 0) {
    // Child: Just print state
    getprocinfo(&info);
    printf("Child before boost: queue=%d\n", info.queue_level);
    
    // Let parent boost us
    sleep(2);
    
    getprocinfo(&info);
    printf("Child after boost: queue=%d\n", info.queue_level);
    exit(0);
  } else {
    // Parent
    sleep(1);
    printf("Parent: Calling system-wide boost\n");
    boostproc(0);  // Boost all
    
    int status;
    wait(&status);
    printf("System-wide boost test completed\n");
  }
}

int
main(int argc, char *argv[])
{
  printf("\n========================================\n");
  printf("Week 3: MLFQ Comprehensive Test Suite\n");
  printf("========================================\n\n");
  
  if(argc < 2) {
    printf("Usage: mlfq_test <test_number>\n");
    printf("  1: CPU demotion test\n");
    printf("  2: I/O fairness test\n");
    printf("  3: Mixed workload test\n");
    printf("  4: Priority boost test\n");
    printf("  5: Manual priority boost test\n");
    printf("  6: System-wide boost test\n");
    printf("  all: Run all tests\n");
    exit(1);
  }
  
  int test = atoi(argv[1]);
  
  if(test == 1 || argv[1][0] == 'a') {
    test_cpu_demotion();
  }
  
  if(test == 2 || argv[1][0] == 'a') {
    test_io_fairness();
  }
  
  if(test == 3 || argv[1][0] == 'a') {
    test_mixed_workload();
  }
  
  if(test == 4 || argv[1][0] == 'a') {
    test_priority_boost();
  }
  
  if(test == 5 || argv[1][0] == 'a') {
    test_manual_boost();
  }
  
  if(test == 6 || argv[1][0] == 'a') {
    test_system_boost();
  }
  
  printf("\n========================================\n");
  printf("All tests completed!\n");
  printf("========================================\n\n");
  
  exit(0);
}
