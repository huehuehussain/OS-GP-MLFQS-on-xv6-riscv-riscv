# Week 2: MLFQ Scheduler Core Implementation

## Overview
Week 2 implements the complete MLFQ scheduler logic with queue management, time quantum enforcement, and demotion. The scheduler now prioritizes processes based on their queue level and enforces exponential time quanta.

## Architecture

### Queue Structure (kernel/proc.h & kernel/proc.c)
```c
struct runqueue {
  struct proc *head;      // Front of queue for dequeuing
  struct proc *tail;      // Back of queue for enqueuing
} runqueues[MLFQ_LEVELS];  // 4 priority queues (0=highest)
```

### Process Extended Fields (kernel/proc.h)
- `queue_next`: Pointer for queue linking
- `queue_level`: Current priority level (0-3)
- `time_in_queue`: Ticks spent in current level
- `time_slices`: Total time slices received
- `entered_queue_tick`: When process entered current queue

### Global State (kernel/proc.c)
- `runqueues[MLFQ_LEVELS]`: Array of queues
- `ticks_since_boost`: Counter for priority boost timing
- `mlfq_lock`: Spinlock for queue protection

## Implementation Details

### 1. Queue Management Functions (kernel/proc.c)

#### get_quantum(int level)
Returns time quantum for a given level:
- Level 0: 2 ticks
- Level 1: 4 ticks
- Level 2: 8 ticks
- Level 3: 16 ticks

#### enqueue(struct proc *p)
Adds process to tail of its current queue level.
Called when:
- Fork creates new process (at level 0)
- wakeup() brings process from SLEEPING to RUNNABLE
- Process yields after using time quantum

#### dequeue(int level)
Removes process from front of specified queue level.
Called by scheduler to select next process.

#### dequeue_specific(struct proc *p)
Removes specific process from any position in queue.
Used for priority boosts and cleanup.

#### demote_process(struct proc *p)
Moves process to next lower priority level (queue_level++).
Triggered when process uses full time quantum.
Prevents starvation of high-priority processes by limiting CPU time.

#### priority_boost(void)
Called every BOOST_INTERVAL (100 ticks).
Moves ALL RUNNABLE processes back to level 0.
Prevents starvation of low-priority processes.
Resets ticks_since_boost counter.

### 2. Process Lifecycle Integration

#### allocproc() - kernel/proc.c
- Initialize `queue_next = 0` (not in queue initially)
- Set `queue_level = 0` (start at highest priority)
- Other MLFQ fields reset to 0

#### freeproc() - kernel/proc.c
- Reset `queue_next = 0` on cleanup

#### kfork() - kernel/proc.c
- After setting `state = RUNNABLE`, call `enqueue(np)`
- New processes enter queue level 0 (highest priority)

#### wakeup() - kernel/proc.c
- When changing state from SLEEPING to RUNNABLE, call `enqueue(p)`
- Moves process from wait channels to scheduler queues

### 3. Scheduler Modification (kernel/proc.c)

New algorithm:
```
for each tick:
  1. Check if BOOST_INTERVAL elapsed, call priority_boost() if needed
  2. Iterate through queues 0 to 3 (highest to lowest priority)
  3. For each level:
     - Dequeue one process
     - If RUNNABLE: execute it, then re-enqueue if still RUNNABLE
     - If not RUNNABLE: skip re-queueing
  4. If any process found to execute, restart from level 0
  5. If no process found: wait (WFI) for interrupt
```

Key features:
- Strict priority: always checks higher levels first
- Round-robin within each level: processes cycle through queues
- Re-enqueue after execution: maintains fair distribution
- State awareness: doesn't re-queue sleeping/zombie processes

### 4. Time Quantum Enforcement (kernel/trap.c)

Modified clockintr() function:
```
For each clock tick:
  1. Increment global ticks counter
  2. Increment ticks_since_boost (for boost tracking)
  3. If current process is RUNNING:
     - Increment p->time_in_queue
     - Get quantum for current level
     - If time_in_queue >= quantum:
       * Reset time_in_queue to 0
       * If not at lowest level: demote (queue_level++)
       * Yield to scheduler (forces reschedule)
```

This enforces exponential time quanta:
- Level 0: max 2 ticks before demotion
- Level 1: max 4 ticks before demotion
- Level 2: max 8 ticks before demotion
- Level 3: max 16 ticks before demotion (lowest level)

### 5. Declarations and Forward References (kernel/defs.h)

Added function declarations:
- `int get_quantum(int)`
- `void enqueue(struct proc*)`
- `struct proc* dequeue(int)`
- `void dequeue_specific(struct proc*)`
- `void demote_process(struct proc*)`
- `void priority_boost(void)`

## Scheduler Behavior

### Process Priority System
```
HIGH PRIORITY                        LOW PRIORITY
    Q0 (2 ticks)                        Q3 (16 ticks)
    Q1 (4 ticks)                        Q2 (8 ticks)
         ↓                                ↑
         ← Demotion (uses full quantum)
         → Promotion (yields early/I/O)
```

### Scheduling Guarantees
1. **Priority**: Higher queue levels always execute before lower levels
2. **Fairness**: Round-robin within same priority level
3. **Progress**: Exponential quanta prevent excessive CPU usage at higher levels
4. **Starvation Prevention**: Priority boost every 100 ticks

### Process State Transitions
```
NEW PROCESS (fork)
    ↓
enqueue(L0) [RUNNABLE, in queue]
    ↓
dequeue(L0) → RUNNING [executing]
    ↓
[After using time quantum]
    ↓
demote → queue_level++ → enqueue(L1) [RUNNABLE, at L1]
    ↓
[After 100 ticks]
    ↓
priority_boost() → queue_level = 0 → enqueue(L0) [back to highest]
```

## Files Modified

### kernel/proc.h
- Added `struct proc *queue_next` field
- Already had MLFQ constants and state fields

### kernel/proc.c
- Added `struct runqueue` declaration and `runqueues[MLFQ_LEVELS]`
- Added `uint ticks_since_boost` global counter
- Added `struct spinlock mlfq_lock`
- Implemented 6 new queue management functions
- Modified allocproc() to initialize queue_next
- Modified freeproc() to reset queue_next
- Modified kfork() to enqueue new processes
- Modified wakeup() to enqueue awakened processes
- Completely rewrote scheduler() to use MLFQ queues

### kernel/trap.c
- Added `extern uint ticks_since_boost` declaration
- Modified clockintr() to:
  - Increment ticks_since_boost
  - Enforce time quantum on running process
  - Trigger demotion when quantum expired
  - Call yield() to force rescheduling

### kernel/defs.h
- Added 6 new function declarations for MLFQ queue management

## Testing Strategy

### Manual Testing (scheduler_demo.c)
The existing test program demonstrates:
1. Process creation and enqueueing
2. Queue level observation via getprocinfo()
3. Behavior differences between process types

### Functional Verification
Expected behaviors:
1. CPU-bound processes should gradually demote to lower queues
2. I/O-bound processes should stay at higher queues
3. All processes should eventually return to level 0 after boost
4. No process should be starved indefinitely

### Compilation
- All queue functions properly declared in defs.h
- All extern references properly resolved
- No circular dependencies
- Type-safe queue operations

## Time Complexity
- `enqueue(p)`: O(1) - direct tail insertion
- `dequeue(level)`: O(1) - direct head removal
- `dequeue_specific(p)`: O(NPROC) worst case - linear scan
- `demote_process(p)`: O(NPROC) - calls dequeue_specific
- `priority_boost()`: O(NPROC) - iterates all processes
- `scheduler()`: O(NPROC) average - processes in queues

## Correctness Invariants

1. **Queue Consistency**: Every RUNNABLE process is in exactly one queue
2. **Level Validity**: queue_level always in [0, MLFQ_LEVELS-1]
3. **Time Accounting**: time_in_queue < quantum for level
4. **State Coherence**: Process state and queue membership synchronized

## Design Decisions

### Why Dequeue at Start of Iteration?
Dequeuing before checking state avoids race conditions. Process can change state during execution. Re-enqueueing only if still RUNNABLE.

### Why Priority Boost?
Prevents indefinite starvation. Without boost, low-priority processes might never execute if high-priority ones keep arriving.

### Why Exponential Quanta?
- Prevents CPU hogs at high priority from blocking everyone
- Allows I/O-bound processes to get CPU when needed
- Degrades gracefully: CPU-bound work still progresses at L3

### Why Break After First Process?
Ensures we restart priority check from level 0. Maintains strict priority scheduling while allowing other levels to make progress.

## Future Enhancements

1. Optimize dequeue_specific() with back-pointers
2. Add per-queue spinlocks for parallelism
3. Implement dynamic quantum adjustment
4. Add statistics collection for scheduler analysis
5. Optional boostproc() syscall for manual boost

## Summary

Week 2 completes the core MLFQ scheduler implementation:
- 4-level priority queues with exponential time quanta (2,4,8,16)
- Automatic demotion when time quantum expires
- Priority boost every 100 ticks prevents starvation
- Strict priority scheduling with round-robin within levels
- Fully integrated with xv6 process lifecycle

The scheduler now properly manages process priorities and prevents resource starvation while maintaining CPU efficiency.
