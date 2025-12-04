# Week 3 Quick Reference

## Testing Commands

### Test Individual Features
```bash
mlfq_test 1    # CPU demotion test
mlfq_test 2    # I/O fairness test
mlfq_test 3    # Mixed workload test
mlfq_test 4    # Priority boost test
mlfq_test 5    # Manual boost test
mlfq_test 6    # System-wide boost test
mlfq_test all  # Run all tests
```

### View Statistics
```bash
mlfq_stats     # Display scheduler statistics
```

## Syscall API

### getschedulerstats(struct mlfq_stats *stats) ✨ NEW

**Purpose:** Retrieve real-time scheduler statistics

**Parameters:**
- `stats`: Pointer to mlfq_stats structure (output parameter)

**Return Values:**
- `0`: Success
- `-1`: Failed to copy statistics

**Data Structure:**
```c
struct mlfq_stats {
  uint64 total_schedules;           // Total scheduler cycles
  uint64 total_boosts;              // Total boost events
  uint64 total_demotions;           // Total demotions
  uint64 level_queue_count[4];      // Processes in each queue (Q0-Q3)
  uint64 level_schedules[4];        // Schedules per queue (Q0-Q3)
};
```

**Example Usage:**
```c
#include "user.h"

int main() {
  struct mlfq_stats stats;
  
  if(getschedulerstats(&stats) < 0) {
    printf("Error: Failed to get statistics\n");
    exit(1);
  }
  
  printf("Total Schedules: %ld\n", stats.total_schedules);
  printf("Total Boosts: %ld\n", stats.total_boosts);
  printf("Total Demotions: %ld\n", stats.total_demotions);
  
  for(int i = 0; i < 4; i++) {
    printf("Queue %d: count=%ld, schedules=%ld\n", 
           i, stats.level_queue_count[i], stats.level_schedules[i]);
  }
}
```

### boostproc(int pid)

**Purpose:** Manually boost process priority

**Parameters:**
- `pid = 0`: Boost all processes to Q0
- `pid > 0`: Boost specific process with given PID

**Return Values:**
- `0`: Success
- `-1`: Process not found (if pid > 0)

**Example Usage:**
```c
int main() {
  int pid = fork();
  if(pid == 0) {
    // Child process
    for(int i = 0; i < 1000000000; i++);  // CPU work
    exit(0);
  } else {
    sleep(1);
    boostproc(pid);  // Boost child to high priority
    wait(0);
  }
}
```

## Expected Scheduler Behavior

### CPU-Bound Process Timeline
```
Time: 0    | Q0 (2 ticks) | 2 ticks used → demotion
Time: 2    | Q1 (4 ticks) | 4 ticks used → demotion
Time: 6    | Q2 (8 ticks) | 8 ticks used → demotion
Time: 14   | Q3 (16 ticks)| 16 ticks used → stays
Time: ~100 | Priority Boost Event! → Q0
Time: 100+ | Q0 (2 ticks) | (cycle repeats)
```

### I/O-Bound Process Timeline
```
Time: 0    | Q0 (2 ticks) | yields early on sleep → stays Q0
Time: 5    | Q0 (2 ticks) | yields early on sleep → stays Q0
Time: 10   | Q0 (2 ticks) | (remains in high priority throughout)
```

## Statistics Interpretation

### Using mlfq_stats Tool

**Running the Tool:**
```bash
mlfq_stats     # Display real kernel statistics
```

**Sample Output:**
```
=== MLFQ Scheduler Statistics ===

Total Events:
  Total Schedules: 50000
  Total Boosts: 500
  Total Demotions: 2500

Queue Distribution (at sample time):
  Queue 0 (2-tick): 3 processes
  Queue 1 (4-tick): 5 processes
  Queue 2 (8-tick): 7 processes
  Queue 3 (16-tick): 10 processes

Scheduler Cycles per Queue:
  Queue 0 (2-tick): 15000 cycles
  Queue 1 (4-tick): 12000 cycles
  Queue 2 (8-tick): 14000 cycles
  Queue 3 (16-tick): 9000 cycles
```

### Interpreting Real Statistics

#### CPU-Bound Workload Indicators
- **High total_demotions** relative to total_schedules
- **High level_schedules[3]** (lowest queue processes most scheduled)
- **Many processes in Queue 3**
- **Meaning:** Processes using full quantum - lots of CPU-bound work

#### I/O-Bound Workload Indicators
- **Low total_demotions** relative to total_schedules
- **High level_schedules[0]** (high priority processes most scheduled)
- **Few processes in Queue 3**
- **Meaning:** Processes yielding early - lots of I/O work

#### Starvation Prevention Working
- **Regular boost events:** total_boosts increasing steadily
- **Queue 3 processes getting scheduled:** level_schedules[3] > 0
- **No processes stuck for 100+ ticks**
- **Meaning:** Lower priority processes are not starved

### High Demotion Rate
- **Indicator:** Many processes use full time quantum
- **Meaning:** Mostly CPU-bound workload
- **Action:** Check if I/O processes are getting starved

### Low Demotion Rate
- **Indicator:** Few processes use full time quantum
- **Meaning:** Mostly I/O-bound workload
- **Action:** Verify CPU-bound processes are not starving

### Uneven Queue Distribution
- **Indicator:** Large deviation in level_schedules
- **Meaning:** Unfair scheduling or boost not triggering
- **Action:** Check boost frequency, verify demotion logic

### Even Queue Distribution
- **Indicator:** Similar schedules across all queues
- **Meaning:** Fair scheduling, proper demotion/boost
- **Action:** Scheduler working as expected

## Performance Targets

### Target Metrics

| Metric | Target | Notes |
|--------|--------|-------|
| Boost Frequency | ~100 ticks | Every ~100 scheduler cycles |
| Demotion Rate | 2-10 per 1000 | Depends on workload |
| Q0 CPU% | 30-40% | High priority processes |
| Q3 CPU% | 5-20% | CPU-intensive processes |
| Max Queue Deviation | < 10% | Fairness metric |

### Thresholds

```
< 100 schedules    : Insufficient data, keep collecting
100-1000 schedules : Valid sample for analysis
> 1000 schedules   : Comprehensive analysis possible

Deviation > 50%    : WARNING - Unfair scheduling
Deviation 10-50%   : OK - Acceptable variance
Deviation < 10%    : EXCELLENT - Very fair
```

## Debugging

### Issue: No Demotions Detected
```
Symptoms: total_demotions = 0
Causes:
  1. No CPU-bound processes running
  2. demote_process() not being called
  3. Clock interrupt not firing
Debugging:
  - Run mlfq_test 1 to force CPU work
  - Check trap.c clockintr() implementation
  - Verify time_in_queue tracking
```

### Issue: No Boosts Detected
```
Symptoms: total_boosts = 0
Causes:
  1. 100-tick interval not passing
  2. priority_boost() not being called
  3. ticks_since_boost not incrementing
Debugging:
  - Run system for > 100 ticks
  - Check clockintr() boost logic
  - Verify ticks_since_boost in trap.c
```

### Issue: Uneven Queue Distribution
```
Symptoms: level_schedules[0] >> level_schedules[3]
Causes:
  1. Demotion not happening correctly
  2. Boost too frequent (moving back to Q0)
  3. Few CPU-bound processes
Debugging:
  - Check demote_process() logic
  - Verify boost interval (should be 100)
  - Run mlfq_test 3 (mixed workload)
```

### Issue: I/O Process Demoting
```
Symptoms: I/O process in lower queues
Causes:
  1. Process not yielding correctly on sleep
  2. yield() not resetting time_in_queue
  3. sleep() implementation issue
Debugging:
  - Check wakeup() logic
  - Verify time_in_queue reset on yield
  - Monitor I/O process with test 2
```

## Code Structure

### Key Functions

**scheduler()** - Main scheduling loop
- Iterates through priority queues
- Updates statistics
- Selects next process

**demote_process()** - Demotion logic
- Called when time_in_queue >= quantum
- Moves to lower priority queue
- Increments demotion counter

**priority_boost()** - Boost logic
- Called every 100 ticks (in trap.c)
- Moves all processes to Q0
- Resets time_in_queue

**sys_boostproc()** - Manual boost syscall
- Boosts all processes (pid=0) or specific process
- Returns 0 on success, -1 on failure

### Global Variables

```c
// Queues
struct proc *runqueues[4];           // 4 priority queues
struct spinlock mlfq_lock;           // Queue access lock
uint64 ticks_since_boost = 0;        // Boost timer

// Statistics
struct mlfq_stats scheduler_stats;   // Global metrics
struct spinlock stats_lock;          // Stats access lock
uint64 scheduler_cycle_count = 0;    // Cycle counter
```

## Test Program Guide

### mlfq_test.c

**Executable:** mlfq_test

**Functions:**
- `cpu_intensive()` - CPU-bound workload generator
- `io_intensive()` - I/O-bound workload generator
- `test_cpu_demotion()` - Test 1
- `test_io_fairness()` - Test 2
- `test_mixed_workload()` - Test 3
- `test_priority_boost()` - Test 4
- `test_manual_boost()` - Test 5
- `test_system_boost()` - Test 6

### mlfq_stats.c

**Executable:** mlfq_stats

**Functions:**
- `print_header()` - Display title
- `print_summary()` - Global metrics
- `print_queue_stats()` - Per-queue metrics
- `print_analysis()` - Performance analysis
- `print_fairness_analysis()` - Fairness assessment
- `print_recommendations()` - Suggestions

## Integration with Weeks 1-2

### Syscall Numbers
```
SYS_getprocinfo  = 22  (Week 1)
SYS_boostproc    = 23  (Week 3)
```

### Data Structures
```
struct proc
  ├── queue_level (Week 2)           // Current queue
  ├── time_in_queue (Week 2)         // Time in current queue
  ├── queue_next (Week 2)            // Next in queue
  └── ... (process fields)

struct mlfq_stats (Week 3)
  ├── total_schedules
  ├── total_boosts
  ├── total_demotions
  ├── level_queue_count[4]
  └── level_schedules[4]
```

### Modified Functions
```
allocproc()         (Week 2) - Initialize MLFQ fields
freeproc()          (Week 2) - Cleanup on exit
scheduler()         (Week 2/3) - Core scheduling + stats
demote_process()    (Week 2/3) - Demotion + tracking
priority_boost()    (Week 2/3) - Boost + tracking
clockintr()         (Week 2/3) - Quantum enforcement
wakeup()            (Week 2) - Enqueue on wakeup
```

## Quick Compilation Check

```bash
# Compile and check for errors
make clean
make 2>&1 | grep -i error

# If no output, compilation successful
# Build filesystem
make qemu

# Inside xv6:
mlfq_test 1   # Should run without errors
mlfq_stats    # Should show statistics
```

## Performance Tuning

### Adjust Time Quanta (in kernel/proc.h)

Current values:
```c
#define QUANTUM_Q0 2    // Level 0: 2 ticks
#define QUANTUM_Q1 4    // Level 1: 4 ticks
#define QUANTUM_Q2 8    // Level 2: 8 ticks
#define QUANTUM_Q3 16   // Level 3: 16 ticks
```

To make more/less aggressive:
```c
// More aggressive (shorter quanta)
#define QUANTUM_Q0 1
#define QUANTUM_Q1 2
#define QUANTUM_Q2 4
#define QUANTUM_Q3 8

// Less aggressive (longer quanta)
#define QUANTUM_Q0 4
#define QUANTUM_Q1 8
#define QUANTUM_Q2 16
#define QUANTUM_Q3 32
```

### Adjust Boost Interval (in kernel/trap.c)

Current: 100 ticks

To boost more frequently:
```c
if(ticks_since_boost >= 50) {  // Boost every 50 ticks
  priority_boost();
  ticks_since_boost = 0;
}
```

## References

- **Implementation:** WEEK3_IMPLEMENTATION.md
- **Week 2 Details:** WEEK2_IMPLEMENTATION.md
- **Week 1 Details:** WEEK1_SUMMARY.md
- **Viva Preparation:** VIVA_EXPLANATION.txt
