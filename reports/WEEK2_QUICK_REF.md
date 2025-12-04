# Week 2 Quick Reference Guide

## MLFQ Scheduler Constants (kernel/proc.h)
```c
MLFQ_LEVELS = 4              // 4 priority levels
QUANTUM_L0 = 2               // Level 0: 2 ticks
QUANTUM_L1 = 4               // Level 1: 4 ticks
QUANTUM_L2 = 8               // Level 2: 8 ticks
QUANTUM_L3 = 16              // Level 3: 16 ticks
BOOST_INTERVAL = 100         // Boost every 100 ticks
```

## Process Structure Extensions (kernel/proc.h)
```c
struct proc {
  // ... existing fields ...
  int queue_level;           // 0 (highest) to 3 (lowest)
  int time_in_queue;         // Ticks in current level
  uint64 time_slices;        // Total CPU time received
  int entered_queue_tick;    // When entered current level
  struct proc *queue_next;   // Next in queue (NEW for Week 2)
};
```

## Queue Data Structure (kernel/proc.c)
```c
struct runqueue {
  struct proc *head;         // Front (dequeue)
  struct proc *tail;         // Back (enqueue)
} runqueues[MLFQ_LEVELS];    // 4 queues total

uint ticks_since_boost = 0;  // Tracks time since last boost
struct spinlock mlfq_lock;   // Protects queue access
```

## Queue Management Functions (kernel/proc.c)

### int get_quantum(int level)
Returns time quantum for a level.
```
get_quantum(0) → 2
get_quantum(1) → 4
get_quantum(2) → 8
get_quantum(3) → 16
```

### void enqueue(struct proc *p)
Adds process to tail of `runqueues[p->queue_level]`.
Sets `p->queue_next = 0`.
Called from: kfork(), wakeup(), scheduler() (re-enqueue)

### struct proc* dequeue(int level)
Removes and returns head of `runqueues[level]`.
Returns NULL if queue empty.
Called from: scheduler()

### void dequeue_specific(struct proc *p)
Removes specific process from its queue.
Handles head, middle, and tail removals.
Used by: demote_process(), priority_boost()

### void demote_process(struct proc *p)
1. Removes process from current queue
2. Increments p->queue_level (if < MLFQ_LEVELS-1)
3. Resets p->time_in_queue = 0
4. Re-enqueues at new level
Called from: clockintr() when quantum exceeded

### void priority_boost(void)
1. Clears all queues
2. Moves all RUNNABLE processes to level 0
3. Resets ticks_since_boost = 0
Called from: scheduler() when ticks_since_boost >= BOOST_INTERVAL

## Scheduler Algorithm (kernel/proc.c)
```
scheduler() {
  for each tick:
    if (ticks_since_boost >= BOOST_INTERVAL):
      priority_boost()
    
    for level = 0 to 3:
      if (p = dequeue(level)):
        if (p->state == RUNNABLE):
          p->state = RUNNING
          swtch() to p
          p->state = (process updated this)
          if (p->state == RUNNABLE):
            enqueue(p)  // Re-enqueue for next turn
        break  // Go back to level 0 check
    
    if (no process found):
      wfi()  // Wait for interrupt
}
```

## Time Quantum Enforcement (kernel/trap.c)

Modified `clockintr()`:
```c
clockintr() {
  ticks++
  ticks_since_boost++
  
  if (p = myproc()) and (p->state == RUNNING):
    p->time_in_queue++
    quantum = get_quantum(p->queue_level)
    
    if (p->time_in_queue >= quantum):
      p->time_in_queue = 0
      if (p->queue_level < MLFQ_LEVELS - 1):
        p->queue_level++
      yield()  // Force reschedule
}
```

## Process Lifecycle Integration

### allocproc() - Initialize New Process
```c
p->queue_level = 0;          // Start at highest priority
p->time_in_queue = 0;
p->time_slices = 0;
p->entered_queue_tick = 0;
p->queue_next = 0;           // Not in queue yet
```

### kfork() - Create Child Process
```c
np->state = RUNNABLE;
enqueue(np);                 // Add to level 0 queue
return np->pid;
```

### freeproc() - Cleanup on Exit
```c
p->queue_next = 0;           // Clear queue pointer
p->queue_level = 0;
p->time_in_queue = 0;
// ... other cleanup ...
p->state = UNUSED;
```

### wakeup() - Bring Process from Sleep
```c
if (p->state == SLEEPING && p->chan == chan):
  p->state = RUNNABLE;
  enqueue(p);                // Add to queue
```

## Execution Flow Example

### Process Starts
```
fork() → allocproc() → queue_level=0, queue_next=0
       → kfork() → enqueue(p, 0)
       → scheduler dequeues from level 0
       → swtch() to process
```

### Process Uses Full Quantum
```
clockintr() tick 2:
  p->time_in_queue = 2
  quantum = get_quantum(0) = 2
  time_in_queue >= quantum ✓
  p->time_in_queue = 0
  p->queue_level = 1
  yield()
scheduler:
  dequeue returns no more from level 0
  dequeue from level 1 → p
  swtch() to p
```

### Process Issues I/O and Sleeps Early
```
Process in level 0 (quantum = 2):
  Tick 1: time_in_queue = 1
  Tick 2: Calls sleep() before quantum ends
         time_in_queue stays 1
         (stays at level 0 on wakeup)
I/O completes:
  wakeup() → p->state = RUNNABLE
          → enqueue(p, 0)  (still at level 0!)
```

### Priority Boost at 100 Ticks
```
tick 100: scheduler checks ticks_since_boost >= 100
          priority_boost()
            clears all queues
            for each process:
              if (RUNNABLE):
                p->queue_level = 0
                enqueue(p)
            ticks_since_boost = 0
scheduler:
  starts checking from level 0 again
```

## Key Differences from Week 1

### Week 1 (Old Scheduler)
- Linear scan of process table
- No queue management
- No time quantum enforcement
- No demotion logic
- No priority distinction

### Week 2 (New Scheduler)
- Queue-based priority scheduling
- Exponential time quanta
- Automatic demotion on quantum expiration
- Priority boost every 100 ticks
- Strict priority enforcement

## Testing Commands

### Compile and Run
```bash
make
make qemu
```

### Run Test Programs
```bash
scheduler_demo      # Watch queue levels change
test_getprocinfo    # Check process info
```

### Expected Behavior
1. New processes start at queue 0
2. CPU-bound processes demote toward queue 3
3. I/O-bound processes stay at high queues
4. After 100 ticks, all should return to queue 0
5. Processes at higher queues run more frequently

## Debugging Tips

### Check Queue State
Use `getprocinfo()` in test programs to verify:
- queue_level (0-3)
- time_in_queue (should reset when demoting)
- time_slices (total CPU time)

### Verify Boost
Add printf in priority_boost() to see when it triggers:
```c
printf("Priority boost at tick %d\n", ticks);
```

### Trace Demotion
Add printf in clockintr() when demotion occurs:
```c
printf("Process %d demoted from %d to %d\n", 
       p->pid, old_level, p->queue_level);
```

### Check Queue Population
Iterate queues in debugger:
```c
for (i = 0; i < MLFQ_LEVELS; i++) {
  printf("Queue %d: ", i);
  for (p = runqueues[i].head; p; p = p->queue_next)
    printf("%d ", p->pid);
  printf("\n");
}
```

## Function Call Graph

```
scheduler()
  ├─ priority_boost() [every 100 ticks]
  │   ├─ dequeue_specific()
  │   └─ enqueue()
  └─ dequeue(level)
      └─ swtch()

clockintr()
  ├─ get_quantum()
  └─ demote_process()
      ├─ dequeue_specific()
      ├─ enqueue()
      └─ yield()

kfork()
  └─ enqueue()

wakeup()
  └─ enqueue()

allocproc()
  [initialize fields]
```

## Memory Impact

Per-process overhead for Week 2:
- `struct proc *queue_next`: 8 bytes
- No additional globals except runqueues[] and counters

Total heap overhead:
- `struct runqueue runqueues[4]`: 64 bytes (4 × 16 bytes)
- `uint ticks_since_boost`: 4 bytes
- `struct spinlock mlfq_lock`: ~16 bytes

Total: ~84 bytes constant + 8 bytes per process

## Correctness Invariants

1. **Every RUNNABLE process is in exactly one queue**
2. **queue_level is always in [0, MLFQ_LEVELS-1]**
3. **time_in_queue < get_quantum(queue_level)**
4. **If process state changes to RUNNING, it's dequeued**
5. **If process state becomes RUNNABLE, it's enqueued**
6. **ticks_since_boost is in [0, BOOST_INTERVAL-1]**

## Performance Characteristics

- Scheduler overhead: O(1) per context switch
- Boost overhead: O(NPROC) every 100 ticks
- Queue operations: O(1) for enqueue/dequeue
- Overall: Nearly same as original with priority benefits

## Summary Table

| Aspect | Old Scheduler | New Scheduler |
|--------|---------------|---------------|
| Algorithm | Linear scan | Queue-based |
| Priorities | None | 4 levels |
| Time Quantum | 1 tick (implicit) | 2,4,8,16 exponential |
| Demotion | None | After quantum |
| Boost | None | Every 100 ticks |
| Fairness | Round-robin | Priority-aware RR |
| Latency | All same | Priority-based |
| CPU efficiency | Fair for all | Responsive for I/O |
