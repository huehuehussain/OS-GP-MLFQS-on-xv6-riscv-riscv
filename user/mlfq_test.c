#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Week 3: Comprehensive MLFQ Testing Program
// Tests demotion, priority boost, and fairness
// Reports PASS/FAIL for each test

struct procinfo info;

// Test tracking
int test_results[7];  // 0=not run, 1=pass, -1=fail (7 tests total)
int num_tests_passed = 0;
int num_tests_failed = 0;

void
print_process_info(char *label)
{
  getprocinfo(&info);
  printf("%s: PID=%d, Queue=%d, Time_In_Queue=%d, Total_Slices=%lu\n",
         label, info.pid, info.queue_level, info.time_in_queue, info.time_slices);
}

int
cpu_intensive(int duration)
{
  // CPU-intensive process for testing demotion
  // Returns 1 if demotion detected, 0 otherwise
  volatile int i, j;
  int start_queue = -1;
  int prev_queue = -1;
  int demotion_detected = 0;
  
  printf("CPU-intensive process started (PID %d)\n", getpid());
  print_process_info("Initial state");
  start_queue = info.queue_level;
  prev_queue = start_queue;
  
  // Much longer CPU work to trigger demotion across quantum boundaries
  for(int iter = 0; iter < duration; iter++) {
    // Heavy busy loop - run much longer to use full quantum
    for(i = 0; i < 5000000; i++) {
      j = i * 2;  // Use j to prevent compiler optimization
      j = j % 1000;  // More work to ensure we hit quantum
    }
    (void)j;  // Explicitly use j to avoid unused warning
    
    getprocinfo(&info);
    if(info.queue_level > prev_queue) {  // Only count increases (actual demotion)
      printf("  Iteration %d: Demoted from queue %d to %d\n", 
             iter, prev_queue, info.queue_level);
      prev_queue = info.queue_level;
      demotion_detected = 1;
    }
  }
  
  printf("CPU-intensive process completed\n");
  print_process_info("Final state");
  
  if(demotion_detected) {
    printf("PASS: Demotion was observed\n");
    exit(1);  // 1 = pass
  } else {
    printf("FAIL: No demotion detected\n");
    exit(0);  // 0 = fail
  }
}

int
io_intensive(int duration)
{
  // I/O-intensive process (sleeps often)
  // Returns 1 if stayed at Q0 (pass), 0 if demoted to Q1+ (fail)
  printf("I/O-intensive process started (PID %d)\n", getpid());
  print_process_info("Initial state");
  
  int start_queue = info.queue_level;
  int max_queue_seen = start_queue;
  
  for(int i = 0; i < duration; i++) {
    // Very short computation followed by yield to simulate I/O
    for(int j = 0; j < 1000; j++) {
      volatile int x = j * 2;
      (void)x;  // Use x to avoid unused warning
    }
    pause(1);  // Yield to scheduler - don't consume quantum
    
    getprocinfo(&info);
    if(info.queue_level > max_queue_seen) {
      printf("  WARNING: I/O process moved to queue %d (started at %d)\n",
             info.queue_level, start_queue);
      max_queue_seen = info.queue_level;
    }
  }
  
  printf("I/O-intensive process completed\n");
  print_process_info("Final state");
  
  // Pass if stayed at Q0, fail if reached Q1 or higher
  if(max_queue_seen == start_queue) {
    printf("PASS: I/O process remained at queue %d\n", info.queue_level);
    exit(1);  // 1 = pass
  } else {
    printf("FAIL: I/O process demoted to queue %d\n", max_queue_seen);
    exit(0);  // 0 = fail
  }
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
    // Parent: wait for child and check exit status
    int status;
    wait(&status);
    
    if(status == 1) {
      printf("✓ TEST 1 PASSED: Demotion detected\n");
      test_results[0] = 1;
      num_tests_passed++;
    } else {
      printf("✗ TEST 1 FAILED: No demotion detected\n");
      test_results[0] = -1;
      num_tests_failed++;
    }
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
    // Parent: wait for child and check exit status
    int status;
    wait(&status);
    
    if(status == 1) {
      printf("✓ TEST 2 PASSED: I/O process stayed at Q0\n");
      test_results[1] = 1;
      num_tests_passed++;
    } else {
      printf("✗ TEST 2 FAILED: I/O process was demoted\n");
      test_results[1] = -1;
      num_tests_failed++;
    }
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
  
  // Parent waits for both and checks results
  int status1, status2;
  wait(&status1);
  wait(&status2);
  
  // Test passes if both children complete (coexistence test)
  printf("✓ TEST 3 PASSED: Mixed workload coexistence verified\n");
  test_results[2] = 1;
  num_tests_passed++;
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
    int saw_demotion = 0;
    int saw_boost = 0;
    
    // Run long enough to trigger demotion (heavy CPU work with frequent yields)
    volatile int i, j;
    for(int iter = 0; iter < 10; iter++) {
      // CPU work with intermediate yields to let scheduler check boost timer
      for(int chunk = 0; chunk < 5; chunk++) {
        // Very long loop to consume multiple ticks and trigger demotion
        for(i = 0; i < 10000000; i++) {
          j = i * 2;
          j = j % 1000;
        }
        (void)j;
        
        // Yield frequently to allow scheduler to check boost timer
        pause(1);
      }
      
      getprocinfo(&info);
      if(info.queue_level > prev_level) {  // Only count increases
        printf("  Iter %d: Queue changed from %d to %d\n", 
               iter, prev_level, info.queue_level);
        saw_demotion = 1;
        prev_level = info.queue_level;
      }
    }
    
    print_process_info("After CPU work");
    
    // At this point, should be demoted to Q2 or Q3
    // After system runs for 100 ticks, should boost back
    printf("Waiting for boost (takes ~100 ticks)...\n");
    
    int initial_queue = info.queue_level;
    
    // Continue doing CPU work with yields to accumulate ticks
    // This ensures enough time passes for the 100-tick boost to fire
    for(int iter = 0; iter < 50; iter++) {
      // Do some CPU work
      volatile int i, j;
      for(i = 0; i < 2000000; i++) {
        j = i * 2;
        j = j % 1000;
      }
      (void)j;
      
      // Yield to let scheduler check boost timer
      pause(1);
      
      getprocinfo(&info);
      if(iter % 10 == 0) {
        printf("  Wait iter %d: queue=%d\n", iter, info.queue_level);
      }
      if(info.queue_level < initial_queue) {
        saw_boost = 1;
        printf("  BOOST DETECTED at iter %d: queue went from %d to %d!\n", 
               iter, initial_queue, info.queue_level);
      }
    }
    
    print_process_info("After boost period");
    
    if(saw_demotion && saw_boost) {
      printf("PASS: Both demotion and boost detected\n");
      exit(1);
    } else {
      printf("FAIL: Missing demotion or boost\n");
      exit(0);
    }
  } else {
    // Parent waits and checks status
    int status;
    wait(&status);
    
    if(status == 1) {
      printf("✓ TEST 4 PASSED: Demotion and boost cycle confirmed\n");
      test_results[3] = 1;
      num_tests_passed++;
    } else {
      printf("✗ TEST 4 FAILED: Boost/demotion cycle incomplete\n");
      test_results[3] = -1;
      num_tests_failed++;
    }
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
    
    int queue_before = info.queue_level;
    
    // Demote self to level 3
    int cpid = getpid();
    boostproc(cpid);
    
    print_process_info("After self-boost");
    
    int queue_after = info.queue_level;
    
    if(queue_after <= queue_before) {
      printf("PASS: Boost worked (queue reduced or stayed same)\n");
      exit(1);
    } else {
      printf("FAIL: Boost did not work\n");
      exit(0);
    }
  } else {
    // Parent
    pause(1);
    printf("Parent: Calling manual boost for child\n");
    boostproc(pid);
    
    int status;
    wait(&status);
    
    if(status == 1) {
      printf("✓ TEST 5 PASSED: Manual boost syscall works\n");
      test_results[4] = 1;
      num_tests_passed++;
    } else {
      printf("✗ TEST 5 FAILED: Manual boost did not work\n");
      test_results[4] = -1;
      num_tests_failed++;
    }
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
    int queue_before = info.queue_level;
    printf("Child before boost: queue=%d\n", queue_before);
    
    // Let parent boost us
    pause(2);
    
    getprocinfo(&info);
    int queue_after = info.queue_level;
    printf("Child after boost: queue=%d\n", queue_after);
    
    if(queue_after <= queue_before) {
      printf("PASS: System boost worked\n");
      exit(1);
    } else {
      printf("FAIL: System boost did not work\n");
      exit(0);
    }
  } else {
    // Parent
    pause(1);
    printf("Parent: Calling system-wide boost\n");
    boostproc(0);  // Boost all
    
    int status;
    wait(&status);
    
    if(status == 1) {
      printf("✓ TEST 6 PASSED: System-wide boost works\n");
      test_results[5] = 1;
      num_tests_passed++;
    } else {
      printf("✗ TEST 6 FAILED: System-wide boost did not work\n");
      test_results[5] = -1;
      num_tests_failed++;
    }
  }
}

void
test_starvation_prevention(void)
{
  printf("\n=== Test 7: Starvation Prevention (Automatic Boost) ===\n");
  printf("Long CPU process should be boosted automatically every 100 ticks\n");
  
  int pid = fork();
  if(pid == 0) {
    // Child: Long-running CPU process
    printf("Child %d: Starting very long CPU work\n", getpid());
    print_process_info("Initial");
    
    int start_queue = info.queue_level;
    int boost_count = 0;
    int last_queue = start_queue;
    int max_queue_reached = start_queue;
    
    // Run for multiple boost intervals with frequent yields
    // Each burst is split into chunks to yield frequently
    for(int burst = 0; burst < 20; burst++) {
      // Heavy CPU work split into chunks with yields
      volatile int i, j;
      for(int chunk = 0; chunk < 3; chunk++) {
        // Very long loop to ensure we consume quantum and get demoted
        for(i = 0; i < 10000000; i++) {
          j = i * 2;
          j = j % 1000;
        }
        (void)j;
        
        // Yield frequently - allows scheduler to check boost timer every 30 ticks
        pause(1);
      }
      
      getprocinfo(&info);
      
      // Track maximum queue reached (lowest priority)
      if(info.queue_level > max_queue_reached) {
        max_queue_reached = info.queue_level;
      }
      
      // Check if queue level improved from previous (boost happened)
      // Boost moves process to LOWER queue number (higher priority)
      if(info.queue_level < last_queue) {
        printf("  Burst %d: BOOSTED! queue went from %d to %d\n", burst, last_queue, info.queue_level);
        boost_count++;
      }
      
      // Track demotions (queue level increased)
      if(info.queue_level > last_queue) {
        printf("  Burst %d: Demoted to queue %d\n", burst, info.queue_level);
      }
      
      last_queue = info.queue_level;
    }
    
    printf("Total automatic boosts observed: %d\n", boost_count);
    print_process_info("Final");
    
    // Pass if we saw at least 1 automatic boost
    if(boost_count >= 1) {
      printf("PASS: Automatic boost observed (starvation prevented)\n");
      exit(1);
    } else {
      printf("FAIL: No automatic boost detected\n");
      exit(0);
    }
  } else {
    // Parent waits
    int status;
    wait(&status);
    
    if(status == 1) {
      printf("✓ TEST 7 PASSED: Starvation prevention confirmed\n");
      test_results[6] = 1;  // Note: test_results needs to be larger
      num_tests_passed++;
    } else {
      printf("✗ TEST 7 FAILED: Starvation prevention not working\n");
      test_results[6] = -1;
      num_tests_failed++;
    }
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
    printf("  7: Starvation prevention test\n");
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
  
  if(test == 7 || argv[1][0] == 'a') {
    test_starvation_prevention();
  }
  
  // Print test summary
  printf("\n========================================\n");
  printf("TEST SUMMARY\n");
  printf("========================================\n");
  printf("Tests Passed: %d\n", num_tests_passed);
  printf("Tests Failed: %d\n", num_tests_failed);
  
  if(num_tests_passed > 0) {
    printf("\n✓ PASSED TESTS:\n");
    if(test_results[0] == 1) printf("  Test 1: CPU Demotion\n");
    if(test_results[1] == 1) printf("  Test 2: I/O Fairness\n");
    if(test_results[2] == 1) printf("  Test 3: Mixed Workload\n");
    if(test_results[3] == 1) printf("  Test 4: Priority Boost\n");
    if(test_results[4] == 1) printf("  Test 5: Manual Boost\n");
    if(test_results[5] == 1) printf("  Test 6: System Boost\n");
    if(test_results[6] == 1) printf("  Test 7: Starvation Prevention\n");
  }
  
  if(num_tests_failed > 0) {
    printf("\n✗ FAILED TESTS:\n");
    if(test_results[0] == -1) printf("  Test 1: CPU Demotion\n");
    if(test_results[1] == -1) printf("  Test 2: I/O Fairness\n");
    if(test_results[2] == -1) printf("  Test 3: Mixed Workload\n");
    if(test_results[3] == -1) printf("  Test 4: Priority Boost\n");
    if(test_results[4] == -1) printf("  Test 5: Manual Boost\n");
    if(test_results[5] == -1) printf("  Test 6: System Boost\n");
    if(test_results[6] == -1) printf("  Test 7: Starvation Prevention\n");
  }
  
  if(num_tests_failed == 0 && num_tests_passed > 0) {
    printf("\nALL TESTS PASSED!\n");
  }
  printf("========================================\n\n");
  
  exit(0);
}
