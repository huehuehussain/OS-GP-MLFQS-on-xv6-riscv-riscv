================================================================================
                       WEEK 3 VIVA EXPLANATION
                    Testing Framework & Performance Analysis
================================================================================

This document contains viva preparation material for Week 3 components.
For Weeks 1-2, refer to VIVA_EXPLANATION.txt

================================================================================
                          WEEK 3: TESTING FRAMEWORK
================================================================================

BOOSTPROC() SYSCALL - MANUAL PRIORITY CONTROL
==============================================

Q: What is the boostproc() syscall used for?
A: boostproc() provides manual priority boost capability:
   - boostproc(0) boosts all processes to Q0 (emergency priority boost)
   - boostproc(pid) boosts specific process with given PID to Q0
   - Complements automatic 100-tick boost for system-critical tasks
   - Allows administrator control over scheduling

Q: How is boostproc() implemented?
A: In kernel/sysproc.c as sys_boostproc():
   - Takes one argument: pid (0 for all, >0 for specific)
   - If pid == 0: Calls priority_boost() function
   - If pid > 0: Searches process table for matching PID
   - Sets queue_level = 0, time_in_queue = 0
   - Returns 0 on success, -1 if process not found

Q: Where is boostproc() syscall number defined?
A: kernel/syscall.h: #define SYS_boostproc 23
   Syscall numbers: getprocinfo=22, boostproc=23

Q: How is boostproc() exposed to user space?
A: Complete syscall interface:
   1. kernel/sysproc.c: Implements sys_boostproc()
   2. kernel/syscall.c: Adds to dispatcher array
   3. user/usys.pl: Generates stub in usys.S
   4. user/user.h: Declares boostproc() prototype

Q: What's the difference between boostproc(0) and boostproc(pid)?
A: boostproc(0): System-wide boost
   - Boosts ALL processes simultaneously
   - Use for global priority adjustment
   - Prevents system-wide starvation

   boostproc(pid): Targeted boost
   - Boosts only specific process
   - Use for individual priority adjustment
   - Returns -1 if process not found

Q: Why use boostproc() instead of relying on automatic boost?
A: Automatic boost happens every 100 ticks globally
   boostproc() provides:
   - Immediate boost (don't wait 100 ticks)
   - Targeted boost (only processes that need it)
   - Emergency priority adjustment capability
   - System admin control

STATISTICS COLLECTION INFRASTRUCTURE
====================================

Q: What statistics does the MLFQ scheduler collect?
A: struct mlfq_stats contains 5 metrics:
   1. total_schedules: Count of scheduler cycles
   2. total_boosts: Count of priority boost events
   3. total_demotions: Count of demotions
   4. level_queue_count[4]: Processes in each queue
   5. level_schedules[4]: Total schedules per queue level

Q: Where are statistics collected?
A: Three collection points in kernel/proc.c:
   1. scheduler(): Increments total_schedules, level_schedules[]
   2. priority_boost(): Increments total_boosts
   3. demote_process(): Increments total_demotions

Q: How are statistics synchronized?
A: struct spinlock stats_lock:
   - Initialized in procinit()
   - Protects concurrent access to scheduler_stats
   - Prevents corruption from parallel updates

Q: Why collect statistics?
A: Provides visibility into scheduler behavior:
   - Verify demotion logic working
   - Verify boost frequency correct
   - Analyze fairness (queue distribution)
   - Detect starvation or imbalance
   - Debug scheduling anomalies

Q: What does high demotion rate indicate?
A: Indicates mostly CPU-bound processes:
   - Normal rate: 1-5% of scheduler cycles
   - High rate (>10%): Many CPU-bound processes
   - Low rate (<1%): Mostly I/O-bound workload
   - Very high (>20%): Potential for starvation

Q: What does boost frequency indicate?
A: Shows how often priority boost occurs:
   - Expected: ~1 boost per 100 scheduler cycles
   - More frequent: System running very fast
   - Less frequent: System idle or few processes
   - 0 boosts: Boost logic broken

Q: How do you interpret level_schedules[]?
A: Shows CPU share per priority level:
   - level_schedules[0]: Q0 (high priority)
   - level_schedules[1]: Q1 (medium-high)
   - level_schedules[2]: Q2 (medium-low)
   - level_schedules[3]: Q3 (lowest priority)
   
   Expected: 30% > 36% > 24% > 10%

TEST PROGRAMS - VALIDATION
==========================

Q: What are the 6 test programs?
A: mlfq_test provides 6 comprehensive tests:
   1. CPU-Bound Demotion: Verify demotion progression
   2. I/O Fairness: Verify I/O priority preservation
   3. Mixed Workload: Verify CPU/I/O fairness
   4. Priority Boost: Verify automatic boost
   5. Manual Boost: Verify boostproc(pid)
   6. System Boost: Verify boostproc(0)

Q: How does Test 1 (CPU Demotion) work?
A: 1. Fork creates child process
   2. Child runs CPU-intensive loops
   3. Monitor queue level each iteration
   4. Expected: Q0 → Q1 → Q2 → Q3
   5. Verifies: demote_process() works

Q: How does Test 2 (I/O Fairness) work?
A: 1. Fork creates child process
   2. Child alternates CPU work with sleep()
   3. Monitor queue level across cycles
   4. Expected: Stays at Q0 (no demotion)
   5. Verifies: I/O processes remain high priority

Q: How does Test 3 (Mixed Workload) work?
A: 1. Fork CPU-bound child (Child 1)
   2. Fork I/O-bound child (Child 2)
   3. Both run concurrently
   4. Expected: Fair CPU time between types
   5. Verifies: Fairness in mixed workloads

Q: How does Test 4 (Priority Boost) work?
A: 1. Fork CPU-bound child
   2. Let child demote to Q3
   3. Wait for 100-tick boost
   4. Expected: Child returns to Q0
   5. Verifies: Automatic boost works

Q: How does Test 5 (Manual Boost) work?
A: 1. Fork creates child process
   2. Parent calls boostproc(child_pid)
   3. Expected: Child moved to Q0
   4. Verifies: Targeted boost works

Q: How does Test 6 (System Boost) work?
A: 1. Fork creates child
   2. Parent calls boostproc(0)
   3. Expected: All at Q0
   4. Verifies: System-wide boost works

Q: How do you run tests?
A: mlfq_test <number>
   1-6: Individual tests
   all: All tests (2-3 minutes)

Q: What does mlfq_stats tool do?
A: Displays scheduler statistics:
   - Global metrics
   - Per-queue statistics
   - Performance analysis
   - Fairness assessment
   - Visual distribution

FAIRNESS & PERFORMANCE
=====================

Q: How do you measure fairness?
A: Compare actual vs expected distribution:
   1. Expected = total_schedules / 4
   2. Actual = level_schedules[i]
   3. Deviation = max(|actual - expected|)
   4. Good: deviation < 1/3 average
   5. Excellent: deviation < 10%

Q: What CPU share should each queue get?
A: Expected:
   Q0: 30-40% (high priority)
   Q1: 25-35% (medium-high)
   Q2: 15-25% (medium-low)
   Q3: 5-20% (CPU-intensive)

Q: What indicates problems?
A: Warning signs:
   - Deviation > 50% (starvation)
   - Q3 < 5% (CPU-bound starving)
   - Q0 > 70% (I/O hogging)
   - 0% demotion (quantum broken)
   - 0 boosts (timer broken)

INTEGRATION WITH WEEKS 1-2
==========================

Q: How does Week 3 integrate with Week 2?
A: Week 2 provides core, Week 3 adds testing:
   - Week 2: Queue management, scheduler
   - Week 3: Statistics, tests, manual boost
   - Uses all Week 2 functions
   - Adds collection hooks

Q: How does Week 3 integrate with Week 1?
A: Week 1 provides data structures:
   - Week 1: getprocinfo() syscall
   - Week 3: Tests use getprocinfo()
   - No changes to Week 1 code

Q: Is Week 3 optional?
A: Functionally, yes - system works without it
   But Week 3 provides:
   - Testing capability
   - Performance visibility
   - Emergency boost control
   - Debugging tools

IMPLEMENTATION DETAILS
======================

Q: Where is boostproc() code?
A: kernel/sysproc.c, sys_boostproc():
   - Parse pid argument
   - If 0: boost all
   - If >0: search and boost specific
   - Proper locking
   - Error handling

Q: Where are statistics collected?
A: Three locations:
   1. scheduler(): total_schedules, level_schedules[]
   2. demote_process(): total_demotions
   3. priority_boost(): total_boosts

Q: How is synchronization handled?
A: struct spinlock stats_lock:
   - Protects scheduler_stats
   - Prevents data corruption
   - Proper acquire/release

COMMON ISSUES
=============

Q: If Test 1 fails (no demotion)?
A: Check:
   1. trap.c increments time_in_queue
   2. demote_process() called correctly
   3. Quantum values in proc.h
   4. Clock interrupt working

Q: If Test 2 fails (I/O demoted)?
A: Check:
   1. wakeup() preserves priority
   2. yield() resets time_in_queue
   3. sleep() calls yield()
   4. Context switch handling

Q: If statistics show zeros?
A: Check:
   1. scheduler() increments counters
   2. stats_lock initialized
   3. demote_process() works
   4. priority_boost() works

Q: If boostproc() returns -1?
A: Check:
   1. Process exists
   2. PID lookup logic
   3. Process table search
   4. Return value handling

================================================================================
                    GET_SCHEDULER_STATS() SYSCALL ✨ NEW
================================================================================

Q: What is get_scheduler_stats() syscall?
A: Retrieves real-time scheduler statistics from kernel to user-space:
   - total_schedules: Total scheduler cycles
   - total_boosts: Priority boost events
   - total_demotions: Process demotions
   - level_queue_count[4]: Processes in each queue
   - level_schedules[4]: Executions per queue
   
Q: What's SYS_getschedulerstats number?
A: 24 (in kernel/syscall.h)
   Syscall numbers: getprocinfo=22, boostproc=23, getschedulerstats=24

Q: Why need get_scheduler_stats() when we have mlfq_test?
A: mlfq_test shows real-time behavior. get_scheduler_stats() shows:
   - Aggregate metrics (total events)
   - Queue distribution (how many processes per level)
   - Fairness analysis (execution counts per queue)
   - Post-test analysis (what happened overall)

Q: How does sys_getschedulerstats() work?
A: Implementation in kernel/sysproc.c:
   1. Extract user pointer with argaddr()
   2. Acquire stats_lock spinlock
   3. Copy kernel's scheduler_stats to local variable
   4. Release stats_lock
   5. Use copyout() to transfer to user-space
   6. Return 0 success or -1 failure

Q: Why use stats_lock?
A: Synchronization - prevents race conditions:
   - Scheduler thread updates stats continuously
   - User thread reads stats via syscall
   - Lock ensures consistent snapshot
   - Prevents partial/corrupted reads

Q: Why use copyout()?
A: Memory safety - kernel and user have separate page tables:
   - copyout() validates user pointer
   - Checks page permissions
   - Safely transfers data across boundary
   - Fails gracefully on invalid pointer

Q: What's returned by getschedulerstats()?
A: In user space (user/user.h):
   struct mlfq_stats {
     uint64 total_schedules;
     uint64 total_boosts;
     uint64 total_demotions;
     uint64 level_queue_count[4];
     uint64 level_schedules[4];
   };

Q: How do you use get_scheduler_stats() in C?
A: Example:
   struct mlfq_stats stats;
   if(getschedulerstats(&stats) < 0) {
     printf("Failed\n");
     exit(1);
   }
   printf("Total schedules: %ld\n", stats.total_schedules);

Q: What does mlfq_stats program do?
A: User-space tool that:
   1. Calls getschedulerstats() syscall
   2. Retrieves REAL kernel statistics
   3. Displays in human-readable format
   4. Interprets fairness and performance
   5. Shows CPU-bound demotion evidence
   6. Shows I/O-bound fairness evidence

Q: How do you interpret statistics?
A: Example analysis:
   - High total_demotions: CPU-bound workload
   - High level_schedules[0]: I/O-bound processes
   - Even distribution: Fair scheduling
   - Boost count ~= runtime/100: Boost working

Q: What if total_schedules keeps growing?
A: Normal - increments every scheduler cycle.
   Use snapshot comparison to measure events:
   - Get stats at time T1
   - Run test
   - Get stats at time T2
   - Calculate differences (T2 - T1)

Q: Can get_scheduler_stats() fail?
A: Yes, returns -1 if:
   - Invalid pointer (&stats not writable)
   - copyout() fails (rare in normal operation)
   - Page fault on user pointer
   
Q: How does get_scheduler_stats() relate to boostproc()?
A: Complementary:
   - boostproc(): Change scheduler behavior (action)
   - get_scheduler_stats(): Observe behavior (measurement)
   - Together: Complete testing framework

STATISTICS INTERPRETATION GUIDE
================================

Q: What indicates successful CPU demotion?
A: After CPU-bound test:
   - total_demotions > 0 (demotions happened)
   - level_schedules[3] high (low priority executed)
   - level_schedules[0] medium (high priority exists)
   - Progression visible in real-time tests

Q: What indicates I/O fairness?
A: After I/O-bound test:
   - total_demotions low or zero (no demotion)
   - level_schedules[0] dominates (stayed high)
   - level_queue_count[0] > 0 (in high queue)
   - Quick response to I/O

Q: What indicates starvation prevention?
A: After long test:
   - total_boosts > 0 (boosts happened)
   - total_boosts ~= (total_ticks / 100) (right frequency)
   - All queues have executions (none starved)
   - Lower queue processes still scheduled

Q: What does unbalanced distribution mean?
A: Example: level_schedules = [30000, 5000, 2000, 1000]
   - Indicates priority starvation
   - Check if boost frequency too low
   - Verify demotion logic working
   - May need shorter boost interval

Q: What does even distribution mean?
A: Example: level_schedules = [10000, 10000, 10000, 10000]
   - Indicates fair scheduling
   - Demotion/boost policy working
   - Good for mixed workloads
   - Expected for balanced systems

VIVA QUESTION PATTERNS ✨ UPDATED
=================================

Q: Typical boostproc() question?
A: Implementation, use cases, pid behavior,
   race conditions, integration with boost

Q: Typical statistics question?
A: What collected, why collect, interpretation,
   how to debug with them, fairness metrics

Q: Typical get_scheduler_stats() question?
A: Purpose, syscall flow, copyout() usage,
   thread safety, statistics meaning

Q: Typical test question?
A: Purpose of each test, how to run, expected
   results, failure debugging, what validates

Q: Typical integration question?
A: How Week 3 uses Weeks 1-2, backward
   compatibility, end-to-end flow

CONCLUSION
==========

Week 3 adds production-grade testing:
- boostproc(): Manual priority control (SYS=23)
- get_scheduler_stats(): Real statistics (SYS=24)
- Statistics: Performance monitoring
- Tests: Comprehensive validation
- Tools: Analysis and debugging

Complete MLFQ scheduler for xv6-RISC-V with comprehensive testing framework.

================================================================================
Document: WEEK3_VIVA_EXPLANATION.md
Version: 2.0 (Updated with get_scheduler_stats)
Status: COMPLETE ✅

Ready for viva defense and deployment with complete syscall documentation.
