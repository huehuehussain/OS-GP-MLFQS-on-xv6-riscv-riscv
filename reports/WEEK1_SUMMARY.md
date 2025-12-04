# Week 1: MLFQ Setup and Design - Implementation Summary

## Overview
Week 1 focuses on understanding the xv6 scheduler, implementing the `getprocinfo()` system call, and designing the MLFQ queue structure. This document summarizes the changes made.

## Changes Made

### 1. Kernel Header Files

#### `kernel/proc.h`
**Added MLFQ Constants:**
```c
#define MLFQ_LEVELS 4              // 4 priority queues
#define QUANTUM_L0 2               // Time quantum for each level
#define QUANTUM_L1 4
#define QUANTUM_L2 8
#define QUANTUM_L3 16
#define BOOST_INTERVAL 100         // Priority boost frequency
```

**Added procinfo Structure:**
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

**Extended struct proc with MLFQ Fields:**
```c
int queue_level;             // Current queue level (0=highest)
int time_in_queue;           // Ticks in current queue
uint64 time_slices;          // Total CPU time slices
int entered_queue_tick;      // Tick when entered current queue
```

#### `kernel/syscall.h`
- Added: `#define SYS_getprocinfo 22`

### 2. Kernel Implementation

#### `kernel/syscall.c`
- Added extern declaration for `sys_getprocinfo()`
- Added syscall to dispatcher array at index `SYS_getprocinfo`

#### `kernel/sysproc.c`
**Implemented `sys_getprocinfo()`:**
```c
uint64
sys_getprocinfo(void)
{
  uint64 addr;
  struct proc *p = myproc();
  struct procinfo info;

  argaddr(0, &addr);

  // Fill structure with process info
  info.pid = p->pid;
  info.state = p->state;
  info.queue_level = p->queue_level;
  info.time_in_queue = p->time_in_queue;
  info.time_slices = p->time_slices;
  safestrcpy(info.name, p->name, sizeof(info.name));

  // Copy to user space
  if(copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
    return -1;

  return 0;
}
```

#### `kernel/proc.c`
**Modified `procinit()`:**
- Initialize all MLFQ fields to 0 for all processes in proc table

**Modified `allocproc()`:**
- Initialize MLFQ fields for newly allocated processes
- Set `queue_level = 0` (highest priority)
- Reset time counters

**Modified `freeproc()`:**
- Reset MLFQ fields when process is freed

### 3. User-Space Interface

#### `user/user.h`
- Added `struct procinfo` definition
- Added function declaration: `int getprocinfo(struct procinfo*);`

#### `user/usys.pl`
- Added: `entry("getprocinfo");` to generate syscall stub

### 4. Test Programs

#### `user/test_getprocinfo.c`
- Simple test program that calls `getprocinfo()` and displays results
- Verifies syscall is working correctly
- Outputs: PID, Name, State, Queue Level, Time in Queue, Time Slices

#### `user/scheduler_demo.c`
- Comprehensive demonstration program
- Creates multiple child processes with different workload patterns:
  - CPU-bound process
  - I/O-bound process
  - Mixed workload process
- Monitors queue levels and time metrics
- Shows expected MLFQ behavior characteristics

### 5. Design Document

#### `MLFQ_DESIGN.md`
Comprehensive design document covering:
- MLFQ Architecture (4-level queue system)
- Time quantum definitions
- Scheduling rules and policies
- Demotion and priority boosting
- System call specification
- Implementation phases
- Testing strategy
- Key design decisions

## Queue Structure Design

### 4-Level Priority System

| Queue | Priority | Quantum | Purpose |
|-------|----------|---------|---------|
| 0 | Highest | 1 tick | Interactive/I/O-bound |
| 1 | High | 2 ticks | Mixed workload |
| 2 | Medium | 4 ticks | CPU-bound (moderate) |
| 3 | Low | 8 ticks | Batch/background |

### MLFQ Rules
1. **Rule 1**: Always run process from highest priority queue
2. **Rule 2**: Within queue, use round-robin with fixed quantum
3. **Rule 3**: Process demoted after using full quantum
4. **Rule 4**: Process stays at level if it yields before quantum expires
5. **Rule 5**: Priority boost every 100 ticks (prevents starvation)

## How to Compile and Test

### Build xv6
```bash
cd /path/to/xv6-riscv
make clean
make
```

### Run Test Programs
```bash
# Simple test
make qemu
# Then in xv6 shell:
test_getprocinfo

# Full demonstration
scheduler_demo

# With argument for direct test mode
scheduler_demo 1
```

### Expected Output Example
```
=== Process Information ===
PID: 6
Process Name: test_getprocinfo
State: 3
Queue Level: 0
Time in Queue: 0 ticks
Total Time Slices: 0
=========================

getprocinfo() syscall works correctly!
```

## Architecture Overview

### Current Implementation Flow
1. **Process Creation**: New processes start at queue level 0 with zero metrics
2. **Scheduler Decision**: Currently round-robin (default xv6 behavior)
3. **getprocinfo() Syscall**: Returns current process metrics
4. **Data Structure**: All MLFQ metadata stored in `struct proc`

### Future Integration Points (Week 2)
- Implement actual queue arrays and management
- Modify `scheduler()` function to use queues
- Add demotion/promotion logic
- Implement time quantum enforcement

## Week 1 Deliverables ✓

- [x] Design document (MLFQ_DESIGN.md)
- [x] System call implementation (getprocinfo)
- [x] System call test program (test_getprocinfo.c)
- [x] Queue structure definition and constants
- [x] MLFQ fields in struct proc
- [x] Comprehensive demo program (scheduler_demo.c)
- [x] Documentation (this file)

## Next Steps (Week 2)

1. Implement queue data structures (arrays/lists)
2. Modify `scheduler()` to dispatch from 4 queues
3. Implement round-robin within each queue
4. Add demotion logic when quantum expires
5. Add yield handling (prevents unnecessary demotion)
6. Create comprehensive test suite

## Notes for Team

### Code Integration Points
- All changes in `kernel/` work with existing code
- User-space interface follows xv6 conventions
- No changes to core scheduler switching logic (yet)
- MLFQ fields are passive; actual scheduling uses default round-robin

### Compilation Notes
- Must regenerate usys.S after modifying usys.pl
- Make clean recommended before rebuilding
- Test compilation with: `make CPUS=1`

### Debugging Tips
- Use `printf()` in kernel for scheduler debugging
- `getprocinfo()` is useful for monitoring process state
- Watch queue levels during test programs to see when they should change

### Repository Structure
```
kernel/
  proc.h          - Added MLFQ constants and structures
  proc.c          - Added MLFQ initialization
  syscall.h       - Added SYS_getprocinfo
  syscall.c       - Added syscall dispatch
  sysproc.c       - Implemented sys_getprocinfo()

user/
  user.h          - Added procinfo struct and function
  usys.pl         - Added getprocinfo entry
  test_getprocinfo.c - Simple test program
  scheduler_demo.c   - Full demonstration
```

---
**Status**: Week 1 Complete  
**Ready for**: Week 2 MLFQ Scheduler Implementation
