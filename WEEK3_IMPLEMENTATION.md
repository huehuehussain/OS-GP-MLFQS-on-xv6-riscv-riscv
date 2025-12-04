# Week 3 Implementation: Testing Framework & Performance Analysis

## Overview

Week 3 focuses on implementing comprehensive testing infrastructure for the MLFQ scheduler. This includes:
- Manual priority boost syscall (boostproc)
- Statistics collection framework
- Test programs to validate scheduler behavior
- Performance analysis tools

## Implemented Components

### 1. boostproc() Syscall (SYS_boostproc = 23)

**Purpose:** Manually boost process priority to highest queue level (0)

**Implementation Location:** kernel/sysproc.c

```c
uint64 sys_boostproc(void) {
  int pid;
  struct proc *p;
  argint(0, &pid);
  
  if(pid == 0) {
    // Boost all processes
    priority_boost();
    return 0;
  } else {
    // Boost specific process
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->pid == pid) {
        p->queue_level = 0;
        p->time_in_queue = 0;
        release(&p->lock);
        return 0;
      }
      release(&p->lock);
    }
    return -1;  // Process not found
  }
}
```

**Usage:**
- `boostproc(0)` - Boost all processes (system-wide boost)
- `boostproc(pid)` - Boost specific process with given PID

**Files Modified:**
- kernel/syscall.h: Added SYS_boostproc = 23
- kernel/syscall.c: Added to dispatcher
- kernel/sysproc.c: Implemented handler
- user/user.h: Added declaration
- user/usys.pl: Added stub generation

### 2. get_scheduler_stats() Syscall (SYS_getschedulerstats = 24) ✨ NEW

**Purpose:** Retrieve real-time scheduler statistics from kernel to user-space for performance analysis

**Implementation Location:** kernel/sysproc.c

```c
uint64 sys_getschedulerstats(void) {
  uint64 stats_ptr;
  argaddr(0, &stats_ptr);
  struct mlfq_stats stats;
  
  // Acquire lock and copy statistics
  acquire(&stats_lock);
  stats = scheduler_stats;
  release(&stats_lock);
  
  // Copy to user space
  if(copyout(myproc()->pagetable, stats_ptr, (char *)&stats, sizeof(stats)) < 0)
    return -1;
  
  return 0;
}
```

**Usage:**
- `getschedulerstats(&stats)` - Retrieve current scheduler statistics into user-space struct
- Returns 0 on success, -1 on failure

**Files Modified:**
- kernel/syscall.h: Added SYS_getschedulerstats = 24
- kernel/syscall.c: Added to dispatcher
- kernel/sysproc.c: Implemented handler
- user/user.h: Added struct mlfq_stats and declaration
- user/usys.pl: Added stub generation

**Key Feature:** Enables real-time performance monitoring. Statistics are collected continuously in kernel and can be queried at any time.

### 3. Statistics Collection Infrastructure

**Data Structure:** struct mlfq_stats (kernel/proc.h)

```c
struct mlfq_stats {
  uint64 total_schedules;           // Total scheduler cycles
  uint64 total_boosts;              // Total boost events
  uint64 total_demotions;           // Total demotions
  uint64 level_queue_count[4];      // Processes in each queue
  uint64 level_schedules[4];        // Schedules per queue
};
```

**Global State:**
- `struct mlfq_stats scheduler_stats` - Global statistics
- `struct spinlock stats_lock` - Synchronization
- `uint64 scheduler_cycle_count` - Cycle counter

**Files Modified:**
- kernel/proc.h: Statistics structure
- kernel/proc.c: Global variables, collection points
- kernel/defs.h: Extern declarations

**Collection Points:**

1. **Scheduler Entry** (scheduler() function)
   ```
   - Increment total_schedules
   - Update level_schedules[level]
   - Persist level_queue_count[level]
   ```

2. **Priority Boost** (priority_boost() function)
   ```
   - Increment total_boosts
   - Move all processes to level 0
   ```

3. **Demotion** (demote_process() function)
   ```
   - Increment total_demotions
   - Move process to lower priority queue
   ```

### 4. Test Programs

#### mlfq_test.c - Comprehensive Test Suite

**Location:** user/mlfq_test.c

**Tests Included:**

1. **Test 1: CPU-Bound Process Demotion**
   - Creates CPU-intensive process
   - Monitors queue level changes
   - Verifies demotion as time quantum is used
   - Expected: Process moves Q0 → Q1 → Q2 → Q3

2. **Test 2: I/O-Bound Process Fairness**
   - Creates I/O-intensive process (yields with sleep)
   - Verifies remains in high priority queue
   - Expected: Process stays at Q0 (yields before quantum expires)

3. **Test 3: Mixed Workload Fairness**
   - Creates both CPU-bound and I/O-bound children
   - Tests fairness in multi-process scenario
   - Expected: Both get fair CPU time

4. **Test 4: Priority Boost Test**
   - CPU-intensive process demotes to lower queues
   - Waits for 100-tick priority boost
   - Verifies boost brings process back to Q0
   - Expected: Automatic boost after ~100 ticks

5. **Test 5: Manual Priority Boost**
   - Tests boostproc() on specific process
   - Parent boosts child process manually
   - Expected: Child immediately moved to Q0

6. **Test 6: System-Wide Boost**
   - Tests boostproc(0) - boost all processes
   - Expected: All processes moved to Q0

**Usage:**
```bash
mlfq_test 1    # CPU demotion test
mlfq_test 2    # I/O fairness test
mlfq_test 3    # Mixed workload test
mlfq_test 4    # Priority boost test
mlfq_test 5    # Manual boost test
mlfq_test 6    # System-wide boost test
mlfq_test all  # Run all tests
```

#### mlfq_stats.c - Statistics Display Tool

**Location:** user/mlfq_stats.c

**Features:**

1. **Global Statistics**
   - Total scheduler cycles
   - Total boost events
   - Total demotion events

2. **Per-Queue Statistics**
   - Processes in each queue
   - Total schedules per queue
   - Percentage of total schedules

3. **Performance Analysis**
   - Demotion rate (demotions per schedule)
   - Boost frequency (schedule cycles per boost)
   - Queue distribution visualization

4. **Fairness Analysis**
   - Expected vs. actual schedules per queue
   - Maximum deviation
   - Fairness assessment (Excellent/Good/Warning)

5. **Recommendations**
   - Data collection status
   - Potential issues to investigate
   - System health indicators

**Output Format:**
```
╔════════════════════════════════════════════════════════════════╗
║           MLFQ Scheduler Statistics Report                     ║
╚════════════════════════════════════════════════════════════════╝

┌─ Global Statistics ──────────────────────────────────────────┐
│ Total Scheduler Cycles:  50000
│ Total Priority Boosts:     500
│ Total Demotions:          1200
└──────────────────────────────────────────────────────────────┘

┌─ Per-Queue Statistics ───────────────────────────────────────┐
│ Queue │ Processes │ Total Schedules │ Schedule %             │
├───────┼───────────┼─────────────────┼──────────────────────┤
│   0   │     2     │      15000      │       30%            │
│   1   │     5     │      18000      │       36%            │
│   2   │     8     │      12000      │       24%            │
│   3   │    15     │       5000      │       10%            │
└───────┴───────────┴─────────────────┴──────────────────────┘

┌─ Performance Analysis ───────────────────────────────────────┐
│ Demotion Rate:           2.40 demotions per 10 schedules  │
│ Boost Frequency:         ~1 boost every 100 schedules      │
│                                                              │
│ Queue Distribution:                                          │
│   Q0: ████████ 30%                                           │
│   Q1: ███████████ 36%                                        │
│   Q2: ██████ 24%                                             │
│   Q3: ██ 10%                                                 │
└──────────────────────────────────────────────────────────────┘
```

## Implementation Details

### Modified Files

**1. kernel/proc.h**
- Added mlfq_stats structure
- All MLFQ constants already present

**2. kernel/proc.c**
- Added global statistics variables (lines ~20-30)
- Modified scheduler() to collect metrics
- Modified demote_process() to track demotions
- Modified procinit() to initialize stats_lock

**3. kernel/syscall.h**
- Added SYS_boostproc = 23

**4. kernel/syscall.c**
- Added sys_boostproc extern declaration
- Added to syscalls dispatcher array

**5. kernel/sysproc.c**
- Implemented sys_boostproc() handler (~40 lines)

**6. kernel/defs.h**
- Added extern declarations for statistics

**7. user/user.h**
- Added boostproc() declaration

**8. user/usys.pl**
- Added entry("boostproc") for stub generation

**9. Makefile**
- Added _mlfq_test target
- Added _mlfq_stats target

## Testing Strategy

### Unit Testing (Individual Components)

```bash
# Test 1: CPU Demotion Behavior
mlfq_test 1
# Expected: Process demotes Q0→Q1→Q2→Q3 as it uses time quantum

# Test 2: I/O-Bound Fairness
mlfq_test 2
# Expected: Process remains at high priority

# Test 5: Manual Boost
mlfq_test 5
# Expected: Process immediately moves to Q0
```

### Integration Testing (Multiple Processes)

```bash
# Test 3: Mixed Workload
mlfq_test 3
# Expected: Both CPU and I/O processes get fair CPU time

# Test 4: Priority Boost
mlfq_test 4
# Expected: Process demoted then boosted after 100 ticks

# Test 6: System-Wide Boost
mlfq_test 6
# Expected: All processes moved to Q0
```

### Performance Analysis

```bash
# Before running tests:
mlfq_stats
# Shows baseline statistics

# Run mixed workload
mlfq_test all

# After running tests:
mlfq_stats
# Shows updated metrics
```

## Expected Results

### Correct Scheduler Behavior

**CPU-Bound Process:**
- Starts at Q0 (2-tick quantum)
- Uses full quantum → demoted to Q1 (4-tick quantum)
- Uses full quantum → demoted to Q2 (8-tick quantum)
- Uses full quantum → demoted to Q3 (16-tick quantum)
- Stays at Q3 until boost event

**I/O-Bound Process:**
- Starts at Q0
- Yields early (on sleep) → remains at Q0
- Consistently gets CPU quickly
- Better responsiveness

**Mixed Workload:**
- CPU-bound process: gradual demotion
- I/O-bound process: maintained in high priority
- Both get reasonable CPU time
- No process starvation

### Statistics Verification

```
✓ total_schedules increases every scheduler cycle
✓ total_boosts increments every 100 ticks
✓ total_demotions increases as processes use full quantum
✓ level_schedules[0] > level_schedules[1] > level_schedules[2] > level_schedules[3]
✓ Demotion rate: reasonable proportion of schedules
✓ Fairness score: deviation < 1/3 of average
```

## Compilation & Execution

### Building

```bash
make clean
make
```

### Running Tests

```bash
# Start xv6
make qemu-gdb  # or appropriate target

# In xv6 shell:
mlfq_test 1    # Run specific test
mlfq_stats     # Display statistics
```

## Week 3 Statistics

### Infrastructure Added
- 1 new syscall (boostproc)
- 1 statistics structure
- 3 global variables
- 2 test programs (~500 lines total)
- Statistics collection hooks in 3 functions
- ~150 lines of kernel code

### Test Coverage
- CPU-bound behavior (demotion)
- I/O-bound behavior (fairness)
- Multi-process fairness
- Automatic priority boost
- Manual priority boost
- System-wide boost

### Performance Metrics Collected
- Total scheduler cycles
- Boost events
- Demotion events
- Per-level scheduling statistics
- Queue depth per level

## Viva Questions & Answers

**Q: How does boostproc() work?**
A: boostproc(0) boosts all processes to Q0; boostproc(pid) boosts specific process.

**Q: What statistics does the scheduler collect?**
A: Total schedules, boosts, demotions, per-level queue counts and schedule counts.

**Q: How do you verify the scheduler is fair?**
A: Compare actual schedule counts per queue against expected values; deviation should be minimal.

**Q: What happens if demotion rate is very high?**
A: May indicate CPU-bound processes are starving I/O processes; check boost frequency.

**Q: Why separate boostproc(0) from individual boost?**
A: Allows system-wide starvation prevention (0) and targeted priority adjustments (pid).

## Conclusion

Week 3 implementation provides:
1. **Testing Framework** - Comprehensive test programs validating scheduler
2. **Statistics Infrastructure** - Real-time performance metrics
3. **Performance Analysis** - Tools to diagnose scheduling issues
4. **Manual Control** - boostproc() for emergency priority adjustments

All components integrate seamlessly with existing Weeks 1-2 implementation while maintaining xv6 conventions and code quality standards.
