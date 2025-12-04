# Multi-Level Feedback Queue (MLFQ) Scheduler Design Document
## xv6-RISC-V Implementation

### 1. Project Overview
This document outlines the design and implementation of a Multi-Level Feedback Queue (MLFQ) scheduler for the xv6 operating system on RISC-V. The MLFQ replaces the default round-robin scheduler with a priority-based scheduling algorithm that dynamically adjusts process priorities based on behavioral patterns.

### 2. MLFQ Architecture

#### 2.1 Queue Structure
The MLFQ scheduler uses **4 priority queues** (levels 0-3):
- **Level 0** (Highest Priority): Interactive/I/O-bound processes
  - Time quantum: 1 tick
  - Purpose: Quick context switches for responsive I/O operations
  
- **Level 1**: Mixed workload processes
  - Time quantum: 2 ticks
  - Purpose: Moderate priority for processes with mixed behavior
  
- **Level 2**: CPU-bound processes (moderate priority)
  - Time quantum: 4 ticks
  - Purpose: Longer slices for CPU work without starvation
  
- **Level 3** (Lowest Priority): Batch/background processes
  - Time quantum: 8 ticks
  - Purpose: Maximum time allocation for batch jobs

#### 2.2 Time Quanta
| Queue Level | Time Quantum | Purpose |
|-------------|--------------|---------|
| 0 | 2 ticks | Interactive processes |
| 1 | 4 ticks | Mixed workload |
| 2 | 8 ticks | CPU-bound (moderate) |
| 3 | 16 ticks | Batch/background |

#### 2.3 Key Parameters
```
MLFQ_LEVELS = 4              // Number of priority queues
QUANTUM_L0 = 2               // Time quantum for level 0 (2 ticks)
QUANTUM_L1 = 4               // Time quantum for level 1 (4 ticks)
QUANTUM_L2 = 8               // Time quantum for level 2 (8 ticks)
QUANTUM_L3 = 16              // Time quantum for level 3 (16 ticks)
BOOST_INTERVAL = 100         // Priority boost every 100 ticks
```

### 3. Process Structure Extensions

#### 3.1 New Fields in `struct proc`
```c
int queue_level;             // Current queue level (0=highest priority)
int time_in_queue;           // Ticks spent in current queue
uint64 time_slices;          // Total CPU time slices received
int entered_queue_tick;      // Tick when process entered current queue
```

#### 3.2 Process Information Structure
For user-space queries via `getprocinfo()` syscall:
```c
struct procinfo {
  int pid;
  int state;
  int queue_level;
  int time_in_queue;
  uint64 time_slices;
  char name[16];
};
```

### 4. Scheduler Behavior Rules

#### 4.1 Scheduling Rules
1. **Rule 1**: Processes are scheduled from the highest priority (lowest queue level) queue first
2. **Rule 2**: Within a queue, use round-robin scheduling with fixed time quantum
3. **Rule 3**: When a process uses its full time quantum, it is demoted to the next lower priority queue
4. **Rule 4**: If a process relinquishes CPU before using full quantum (I/O), it stays at current level
5. **Rule 5**: After BOOST_INTERVAL (100 ticks), all processes are moved to level 0 (priority boost)

#### 4.2 Process Demotion
- Process is demoted when it exhausts its time quantum at current level
- Demotion only occurs down to level 3 (lowest level)
- `time_in_queue` is reset to 0 after demotion
- `entered_queue_tick` is updated

#### 4.3 Priority Boosting (Starvation Prevention)
- Every 100 ticks, all runnable processes are moved to queue level 0
- This prevents starvation of I/O-bound and CPU-bound processes
- Ensures fairness across all priority levels

### 5. System Call: `getprocinfo()`

#### 5.1 Definition
```c
int getprocinfo(struct procinfo *info);
```

#### 5.2 Purpose
- Query process scheduling information
- Useful for testing and debugging MLFQ behavior
- Returns current queue level, time in queue, and total time slices

#### 5.3 Implementation Details
- **Syscall Number**: 22 (SYS_getprocinfo)
- **Parameter**: Pointer to `struct procinfo` in user space
- **Return Value**: 0 on success, -1 on error
- **Operation**: Fills the structure with current process information
- **Note**: Operates on calling process (`myproc()`)

### 6. Implementation Phases

#### Phase 1: Week 1 - Setup and Design (COMPLETED)
- [x] Set up xv6 and understand default scheduler
- [x] Implement `getprocinfo()` system call
- [x] Add MLFQ fields to `struct proc`
- [x] Design queue structure and time quanta
- [x] Create queue scaffolding (data structure foundation)
- [x] Generate this design document

**Deliverables:**
- Design document (this file)
- Syscall implementation tested (getprocinfo.c test program)
- Queue structure defined in proc.h

#### Phase 2: Week 2 - MLFQ Scheduler Core
- [ ] Implement queue data structures (runqueue arrays)
- [ ] Modify scheduler() to handle 4 priority queues
- [ ] Implement round-robin within each level
- [ ] Implement time quantum enforcement
- [ ] Implement process demotion logic
- [ ] Add debug prints for queue statistics

**Deliverables:**
- MLFQ scheduler implementation
- Queue management functions
- Test results showing demotion behavior

#### Phase 3: Week 3 - Boosting and Polishing
- [ ] Implement priority boosting every 100 ticks
- [ ] Add `boostproc()` syscall (optional for testing)
- [ ] Comprehensive testing: CPU-bound vs I/O-bound fairness
- [ ] Performance analysis and optimization
- [ ] Final testing suite

**Deliverables:**
- Final MLFQ code with boosting
- Final report with analysis
- Demo results showing fairness

### 7. Implementation Notes

#### 7.1 Synchronization
- Process lock (`p->lock`) must be held when accessing MLFQ fields
- Scheduler atomicity is maintained through existing spin locks
- No new locks required for MLFQ (uses existing proc table lock)

#### 7.2 Context Switching
- MLFQ integrates with existing context switching mechanism
- No changes to `swtch()` function needed
- Changes localized to scheduler decision logic

#### 7.3 Single-Core Simplification
- Project runs on single CPU (CPUS := 1) for simplicity
- Eliminates CPU affinity concerns
- All scheduling decisions in single `scheduler()` function

### 8. Testing Strategy

#### 8.1 Unit Tests
1. **getprocinfo() functionality**: Verify syscall returns correct values
2. **Queue assignments**: Check processes assigned to correct queue levels
3. **Time quantum enforcement**: Verify processes demote after quantum expires
4. **Round-robin fairness**: Check equal access within queue level

#### 8.2 Integration Tests
1. **CPU-bound processes**: Verify they move down queues over time
2. **I/O-bound processes**: Verify they stay at high priority levels
3. **Mixed workload**: Verify fairness between different process types
4. **Starvation prevention**: Verify priority boost prevents starvation

#### 8.3 Demo Programs
- `test_getprocinfo.c`: Test getprocinfo() syscall
- `cpu_intensive.c`: CPU-bound test process
- `io_intensive.c`: I/O-bound test process
- `scheduler_demo.c`: Mixed workload demonstration

### 9. Key Design Decisions

#### 9.1 Why 4 Queues?
- Provides good balance between scheduling complexity and flexibility
- Allows differentiation between multiple process behaviors
- Empirically proven by prior MLFQ research

#### 9.2 Why These Time Quanta?
- Exponential growth (1, 2, 4, 8) prevents starvation
- Appropriate for both interactive and batch workloads
- Allows tuning without code changes

#### 9.3 Why 100-Tick Boost Interval?
- Prevents long-term starvation of lower queue levels
- Allows sufficient differentiation for behavioral classification
- Can be tuned based on workload characteristics

### 10. Future Enhancements (Optional)
1. Adaptive time quanta based on system load
2. Process priority hints from user applications
3. Per-queue statistics and monitoring
4. Dynamic queue count based on workload
5. Energy-aware scheduling optimizations

### 11. References
- xv6 Book: Chapter 8 (Scheduling)
- "Operating Systems: Three Easy Pieces" - Scheduling Chapter
- Ousterhout, 1982: "Scheduling Techniques for Concurrent Systems"
- Corbató & Vyssotsky, 1965: Time-Sharing Concepts

---

**Design Document Version**: 1.0  
**Last Updated**: Week 1  
**Status**: Foundation Complete, Implementation In Progress
