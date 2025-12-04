# Week 3 Testing Guide

## Overview

This guide provides step-by-step instructions for testing the MLFQ scheduler implementation in Week 3, including how to run individual tests, interpret results, and debug issues.

## Prerequisites

1. xv6-RISC-V with Week 1-3 MLFQ implementation complete
2. All test programs compiled (mlfq_test, mlfq_stats)
3. Makefile updated with test targets

## Test Execution

### Quick Start

```bash
# Build system
cd /path/to/xv6-riscv
make clean
make

# Start QEMU with xv6
make qemu

# Inside xv6 shell, run tests
$ mlfq_test 1
$ mlfq_stats
$ exit
```

## Viewing Scheduler Statistics ✨ NEW

### Using mlfq_stats Tool

**Purpose:** Display real-time scheduler statistics collected from the kernel

**Command:**
```bash
mlfq_stats
```

**Key Features:**
- Retrieves statistics via getschedulerstats() syscall
- Shows actual kernel data (not simulated)
- Can be run alongside tests to monitor behavior
- Useful for post-test analysis

**Example Workflow:**

Terminal 1 (Run long test):
```bash
$ mlfq_test 1    # CPU-bound process demotion (takes ~30 seconds)
```

Terminal 2 (Monitor statistics):
```bash
$ sleep 5        # Wait for test to start
$ mlfq_stats     # Check current statistics
```

**Output Example:**
```
=== MLFQ Scheduler Statistics ===

Total Events:
  Total Schedules: 50000
  Total Boosts: 500
  Total Demotions: 2500

Queue Distribution:
  Queue 0 (2-tick): 3 processes
  Queue 1 (4-tick): 5 processes
  Queue 2 (8-tick): 7 processes
  Queue 3 (16-tick): 10 processes

Scheduler Cycles per Queue:
  Queue 0: 15000
  Queue 1: 12000
  Queue 2: 14000
  Queue 3: 9000
```

### Interpreting Statistics

**For CPU-Bound Test (Test 1):**
- total_demotions should increase (see process move Q0→Q1→Q2→Q3)
- level_schedules[3] should dominate (CPU process in lowest queue)
- level_schedules[0] relatively low

**For I/O-Bound Test (Test 2):**
- total_demotions stays relatively constant
- level_schedules[0] should increase (I/O process stays high priority)
- few processes in Queue 3

**For Mixed Workload Test (Test 3):**
- Balanced demotions (CPU process demoting)
- High level_schedules[0] (I/O process there)
- Boosts occurring regularly (100-tick intervals)

### Real vs Template Data

**⚠️ Critical Difference:**
- mlfq_stats calls **getschedulerstats() syscall** → Real kernel data
- Gets actual statistics from kernel's scheduler_stats structure
- Data updated continuously as scheduler runs
- Demonstrates CPU-bound demotion, I/O fairness, starvation prevention

**Before:** Would show template/example values
**After:** Shows actual kernel behavior during test execution

## Individual Test Descriptions

### Test 1: CPU-Bound Process Demotion

**Purpose:** Verify that CPU-intensive processes are demoted through queue levels as they use their time quantum.

**Command:**
```bash
mlfq_test 1
```

**What Happens:**
1. Fork creates child process
2. Child performs CPU-intensive loops
3. Monitor process queue level over iterations
4. Display initial and final queue levels

**Expected Output:**
```
=== Test 1: CPU-Bound Process Demotion ===
CPU-intensive process started (PID 3)
Initial state: PID=3, Queue=0, Time_In_Queue=0, Total_Slices=0
  Iteration 0: Demoted from queue 0 to 1
  Iteration 1: Demoted from queue 1 to 2
  Iteration 2: Demoted from queue 2 to 3
  Iteration 3: (stays at 3)
  Iteration 4: (stays at 3)
CPU-intensive process completed
Final state: PID=3, Queue=3, Time_In_Queue=?, Total_Slices=?
CPU demotion test completed
```

**Success Criteria:**
- ✓ Process starts at Q0
- ✓ Progresses through Q1, Q2 to Q3
- ✓ Each demotion happens after using full quantum
- ✓ Process eventually settles at Q3

**Debugging:**
- If no demotion: Check demote_process() in proc.c
- If unexpected queue: Check quantum values in proc.h
- If stuck at Q0: Check time_in_queue tracking in trap.c

---

### Test 2: I/O-Bound Process Fairness

**Purpose:** Verify that I/O-intensive processes (those that yield) remain at high priority and don't get demoted.

**Command:**
```bash
mlfq_test 2
```

**What Happens:**
1. Fork creates child process
2. Child performs short CPU work then sleeps (simulates I/O)
3. Monitor queue level across sleep cycles
4. Verify process stays at high priority

**Expected Output:**
```
=== Test 2: I/O-Bound Process Fairness ===
I/O-intensive process started (PID 4)
Initial state: PID=4, Queue=0, Time_In_Queue=0, Total_Slices=0
  Waiting for boost (takes ~100 ticks)...
  (sleep cycles 0-5...)
I/O-intensive process completed
Final state: PID=4, Queue=0, Time_In_Queue=?, Total_Slices=?
  SUCCESS: I/O process remained at queue 0
I/O fairness test completed
```

**Success Criteria:**
- ✓ Process starts at Q0
- ✓ NO demotion message appears
- ✓ Process ends at Q0
- ✓ "SUCCESS: I/O process remained" message

**Debugging:**
- If process demotes: Check sleep() resets time_in_queue
- If stays demoted: Check wakeup() logic in proc.c
- If constant demotion: Check yield() implementation

---

### Test 3: Mixed Workload Fairness

**Purpose:** Verify fair scheduling when both CPU-bound and I/O-bound processes run simultaneously.

**Command:**
```bash
mlfq_test 3
```

**What Happens:**
1. Fork creates CPU-bound child (Child 1)
2. Fork creates I/O-bound child (Child 2)
3. Both run concurrently
4. Both report final queue levels

**Expected Output:**
```
=== Test 3: Mixed Workload Fairness ===
Child 1 (CPU-bound) starting
Child 2 (I/O-bound) starting
Child 1: Process 5: Starting CPU-intensive work
Initial state: PID=5, Queue=0, Time_In_Queue=0, Total_Slices=0
  Iter 0: Queue changed from 0 to 1
  Iter 1: Queue changed from 1 to 2
  (CPU child demoting while I/O child works)
Process 6: I/O-intensive process started (PID 6)
Initial state: PID=6, Queue=0, Time_In_Queue=0, Total_Slices=0
  (I/O child staying high priority)
Mixed workload test completed
```

**Success Criteria:**
- ✓ Both children run without hanging
- ✓ CPU child demotes progressively
- ✓ I/O child remains at Q0
- ✓ Both complete successfully

**Debugging:**
- If one child hangs: Check scheduler for deadlock
- If only one runs: Check fork() implementation
- If both demote: Check I/O-bound detection in wakeup

---

### Test 4: Priority Boost Test

**Purpose:** Verify that automatic priority boost occurs every ~100 ticks and brings demoted processes back to Q0.

**Command:**
```bash
mlfq_test 4
```

**What Happens:**
1. Create CPU-bound process
2. Let it demote to Q3
3. Wait for 100-tick interval to elapse
4. Verify process returns to Q0

**Expected Output:**
```
=== Test 4: Priority Boost Test ===
Starting a CPU-bound process that should demote, then boost
Process 7: Starting CPU-intensive work
Initial state: PID=7, Queue=0, Time_In_Queue=0, Total_Slices=0
  Iter 0: Queue changed from 0 to 1
  Iter 1: Queue changed from 1 to 2
  Iter 2: Queue changed from 2 to 3
  (CPU work continues...)
After CPU work: PID=7, Queue=3, Time_In_Queue=?, Total_Slices=?
Waiting for boost (takes ~100 ticks)...
  Still waiting... queue=3
  Still waiting... queue=3
  Still waiting... queue=0  <- BOOST HAPPENED HERE!
After boost period: PID=7, Queue=0, Time_In_Queue=0, Total_Slices=?
Priority boost test completed
```

**Success Criteria:**
- ✓ Process demotes to Q3
- ✓ After waiting, process returns to Q0
- ✓ Boost message appears at expected time
- ✓ time_in_queue resets to 0

**Debugging:**
- If no boost: Check ticks_since_boost in trap.c
- If boost too early/late: Check 100-tick interval in trap.c
- If other processes boost: That's expected (system-wide boost)
- If stuck at Q3: Check priority_boost() in proc.c

---

### Test 5: Manual Priority Boost Test

**Purpose:** Verify boostproc() syscall can manually boost specific process priority.

**Command:**
```bash
mlfq_test 5
```

**What Happens:**
1. Fork creates child process
2. Parent sleeps to let child get ready
3. Parent calls boostproc(child_pid)
4. Child verifies it moved to Q0

**Expected Output:**
```
=== Test 5: Manual Priority Boost (boostproc syscall) ===
Child process 8 starting
Before manual boost: queue=0
(parent boosting child...)
Child after self-boost: queue=0
Manual boost test completed
```

**Success Criteria:**
- ✓ boostproc() returns successfully
- ✓ Child process is boosted
- ✓ No error messages
- ✓ Test completes normally

**Debugging:**
- If "Process not found": Check sys_boostproc() PID lookup
- If wrong queue: Check boostproc implementation
- If no effect: Verify queue_level actually changes
- If crashes: Check argument parsing in sys_boostproc()

---

### Test 6: System-Wide Priority Boost

**Purpose:** Verify boostproc(0) boosts all processes in the system.

**Command:**
```bash
mlfq_test 6
```

**What Happens:**
1. Fork creates child process
2. Child records initial queue
3. Parent calls boostproc(0) to boost all
4. Child verifies it moved to Q0

**Expected Output:**
```
=== Test 6: System-Wide Priority Boost ===
Testing boostproc(0) - boost all processes
Child before boost: queue=0
(parent calling system-wide boost...)
Child after boost: queue=0
System-wide boost test completed
```

**Success Criteria:**
- ✓ boostproc(0) returns successfully
- ✓ All processes boosted (parent, child, init)
- ✓ No process crashes
- ✓ Test completes

**Debugging:**
- If child not boosted: Check boostproc(0) in sys_boostproc()
- If only some boosted: Check all-process iteration
- If hangs: Check lock management in sys_boostproc()
- If crash: Check bounds checking in process iteration

---

## Statistics Analysis

### Running Statistics Tool

```bash
# After running test
$ mlfq_stats
```

### Interpreting Output

**Example Output:**
```
╔════════════════════════════════════════════════════════════════╗
║           MLFQ Scheduler Statistics Report                     ║
╚════════════════════════════════════════════════════════════════╝

┌─ Global Statistics ──────────────────────────────────────────┐
│ Total Scheduler Cycles:  50000
│ Total Priority Boosts:     500
│ Total Demotions:          1200
└──────────────────────────────────────────────────────────────┘
```

**Analysis:**
- **Cycles = 50000**: System has run for significant time
- **Boosts = 500**: About 1 boost per 100 cycles ✓
- **Demotions = 1200**: About 2.4% demotion rate (reasonable)

```
┌─ Per-Queue Statistics ───────────────────────────────────────┐
│ Queue │ Processes │ Total Schedules │ Schedule %             │
├───────┼───────────┼─────────────────┼──────────────────────┤
│   0   │     2     │      15000      │       30%            │
│   1   │     5     │      18000      │       36%            │
│   2   │     8     │      12000      │       24%            │
│   3   │    15     │       5000      │       10%            │
└───────┴───────────┴─────────────────┴──────────────────────┘
```

**Analysis:**
- **Q0 (30%)**: Higher priority processes getting preference ✓
- **Q1 (36%)**: Most processes passing through Q1 ✓
- **Q2 (24%)**: Longer-running processes ✓
- **Q3 (10%)**: CPU hogs relegated to lowest priority ✓
- **Distribution**: Exponential tail (30 > 36 is OK due to I/O) ✓

```
│ Queue Distribution:                                          │
│   Q0: ████████ 30%                                           │
│   Q1: ███████████ 36%                                        │
│   Q2: ██████ 24%                                             │
│   Q3: ██ 10%                                                 │
```

**Fairness Assessment:**
- Deviation from average (25%): Max ±11% (good fairness)
- Visualization shows expected distribution
- No starvation (Q3 getting 10% of time)

---

## Running All Tests

### Sequential Test Run

```bash
$ mlfq_test all
```

**Time Required:** ~2-3 minutes

**Expected Output:**
```
========================================
Week 3: MLFQ Comprehensive Test Suite
========================================

=== Test 1: CPU-Bound Process Demotion ===
...
CPU demotion test completed

=== Test 2: I/O-Bound Process Fairness ===
...
I/O fairness test completed

=== Test 3: Mixed Workload Fairness ===
...
Mixed workload test completed

=== Test 4: Priority Boost Test ===
...
Priority boost test completed

=== Test 5: Manual Priority Boost (boostproc syscall) ===
...
Manual boost test completed

=== Test 6: System-Wide Priority Boost ===
...
System-wide boost test completed

========================================
All tests completed!
========================================
```

### After All Tests

```bash
$ mlfq_stats
```

At this point, statistics should show significant activity.

---

## Common Issues & Solutions

### Issue: "mlfq_test: command not found"

**Cause:** Test program not compiled or not in PATH

**Solution:**
```bash
# Rebuild filesystem
make clean
make qemu
```

### Issue: "boostproc not found" or "-1" return

**Cause:** Syscall not registered properly

**Debug:**
1. Check kernel/syscall.h has SYS_boostproc = 23
2. Check kernel/syscall.c has dispatcher entry
3. Check kernel/sysproc.c has sys_boostproc() implementation
4. Recompile with `make clean; make`

### Issue: No demotions in Test 1

**Cause:** Quantum enforcement not working

**Debug:**
1. Check trap.c clockintr() increments time_in_queue
2. Check demote_process() is called when time_in_queue >= quantum
3. Add debug output to verify ticks

### Issue: Test hangs indefinitely

**Cause:** Deadlock in queue locks

**Debug:**
1. Check all lock acquire/release pairs match
2. Verify no nested locks on same lock type
3. Check for circular wait in lock dependencies
4. Kill with Ctrl+C and try simpler test

### Issue: Statistics show zeros

**Cause:** Statistics not being collected

**Debug:**
1. Check scheduler() increments total_schedules
2. Check demote_process() increments total_demotions
3. Check priority_boost() increments total_boosts
4. Verify stats_lock initialized in procinit()

### Issue: I/O process getting demoted (Test 2 fails)

**Cause:** Sleep not resetting time_in_queue properly

**Debug:**
1. Check wakeup() in proc.c
2. Verify yield() in swtch.S resets counters
3. Check sleep() implementation
4. Trace process through sleep/wakeup cycle

---

## Performance Benchmarking

### Baseline Collection

```bash
$ mlfq_stats  # Note initial values
Total Scheduler Cycles: 1000
Total Priority Boosts: 10
Total Demotions: 25
```

### Run Heavy Workload

```bash
$ mlfq_test all  # Run all tests
```

### Measure Statistics

```bash
$ mlfq_stats  # Note updated values
Total Scheduler Cycles: 125000
Total Priority Boosts: 1250
Total Demotions: 3000
```

### Calculate Metrics

```
Cycles Added: 124000
Boosts Added: 1240
Demotion Rate: 3000/125000 = 2.4%
Boost Frequency: 125000/1250 = 100 cycles/boost ✓
```

---

## Expected Performance Results

### Correct Scheduler Characteristics

- **Demotion Rate:** 1-5% of cycles
- **Boost Frequency:** ~100-105 cycles between boosts
- **Queue Distribution:** Q0 > Q1 ≥ Q2 >> Q3 (exponential tail)
- **I/O Fairness:** No unintended demotion on sleep
- **CPU Fairness:** CPU hogs not starved (Q3 gets 5-20%)
- **Max Fairness Deviation:** < 30% from average

### Warning Signs

| Symptom | Cause | Fix |
|---------|-------|-----|
| 0 demotions | CPU work not enforcing quantum | Check trap.c |
| 0 boosts | 100-tick timer not working | Check clockintr() |
| Uneven distribution | Queue logic broken | Check enqueue/dequeue |
| High variance | Not enough data | Run longer tests |
| I/O starvation | I/O not staying high priority | Check wakeup() |

---

## Conclusion

This testing guide provides comprehensive coverage of Week 3 MLFQ scheduler testing. By running these tests and analyzing results, you can verify correct scheduler operation, identify issues, and validate that the implementation meets requirements.

**Key Takeaways:**
1. Test 1-4 validate core scheduler behavior
2. Test 5-6 validate manual boost capability
3. Statistics tool provides performance visibility
4. Expected metrics guide interpretation

For detailed implementation info, see WEEK3_IMPLEMENTATION.md
For quick reference, see WEEK3_QUICK_REF.md
