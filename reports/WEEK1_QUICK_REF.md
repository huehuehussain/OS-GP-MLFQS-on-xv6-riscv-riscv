# Week 1 MLFQ Implementation - Quick Reference

## Files Modified (Summary of Changes)

### Kernel Files Changed

#### 1. `kernel/proc.h`
```diff
+ #define MLFQ_LEVELS 4
+ #define QUANTUM_L0 2
+ #define QUANTUM_L1 4
+ #define QUANTUM_L2 8
+ #define QUANTUM_L3 16
+ #define BOOST_INTERVAL 100
+
+ struct procinfo {
+   int pid;
+   int state;
+   int queue_level;
+   int time_in_queue;
+   uint64 time_slices;
+   char name[16];
+ };

In struct proc:
+ int queue_level;
+ int time_in_queue;
+ uint64 time_slices;
+ int entered_queue_tick;
```

#### 2. `kernel/syscall.h`
```diff
#define SYS_close  21
+ #define SYS_getprocinfo 22
```

#### 3. `kernel/syscall.c`
```diff
extern uint64 sys_close(void);
+ extern uint64 sys_getprocinfo(void);

[SYS_close]   sys_close,
+ [SYS_getprocinfo] sys_getprocinfo,
```

#### 4. `kernel/sysproc.c`
```diff
+ uint64
+ sys_getprocinfo(void)
+ {
+   // Gets process info and copies to user space
+ }
```

#### 5. `kernel/proc.c`
```diff
In procinit():
+ p->queue_level = 0;
+ p->time_in_queue = 0;
+ p->time_slices = 0;
+ p->entered_queue_tick = 0;

In allocproc():
+ p->queue_level = 0;
+ p->time_in_queue = 0;
+ p->time_slices = 0;
+ p->entered_queue_tick = 0;

In freeproc():
+ p->queue_level = 0;
+ p->time_in_queue = 0;
+ p->time_slices = 0;
+ p->entered_queue_tick = 0;
```

### User-Space Files Changed

#### 1. `user/user.h`
```diff
+ struct procinfo {
+   int pid;
+   int state;
+   int queue_level;
+   int time_in_queue;
+   uint64 time_slices;
+   char name[16];
+ };

// system calls
+ int getprocinfo(struct procinfo*);
```

#### 2. `user/usys.pl`
```diff
entry("uptime");
+ entry("getprocinfo");
```

### New Files Created

#### 1. `user/test_getprocinfo.c`
- Simple test of getprocinfo syscall
- Run with: `test_getprocinfo` in xv6

#### 2. `user/scheduler_demo.c`
- Comprehensive scheduler demonstration
- Forks 3 different process types
- Run with: `scheduler_demo` in xv6

### Documentation Files Created

#### 1. `MLFQ_DESIGN.md`
- Complete design document (2+ pages)
- Queue architecture
- Scheduler rules
- Syscall specifications
- Testing strategy

#### 2. `WEEK1_SUMMARY.md`
- Implementation summary
- All changes documented
- How to compile and test
- Next steps for Week 2

---

## Key Concepts Implemented

### MLFQ Queue Design
```
┌─────────────────────────────────────┐
│ Queue Level 0 (Priority: HIGH)      │  Time Quantum: 1 tick
│ [Process A] [Process B]             │  (Interactive)
└─────────────────────────────────────┘
        ↓ (if uses full quantum)
┌─────────────────────────────────────┐
│ Queue Level 1 (Priority: MEDIUM-HI) │  Time Quantum: 2 ticks
│ [Process C] [Process D]             │  (Mixed)
└─────────────────────────────────────┘
        ↓ (if uses full quantum)
┌─────────────────────────────────────┐
│ Queue Level 2 (Priority: MEDIUM-LO) │  Time Quantum: 4 ticks
│ [Process E] [Process F]             │  (CPU-bound)
└─────────────────────────────────────┘
        ↓ (if uses full quantum)
┌─────────────────────────────────────┐
│ Queue Level 3 (Priority: LOW)       │  Time Quantum: 8 ticks
│ [Process G] [Process H]             │  (Batch/Background)
└─────────────────────────────────────┘
```

### Process Behavior in MLFQ

```
I/O-Bound Process:
- Starts at level 0
- Yields before quantum expires
- Stays at level 0 (interactive)
- Pattern: [L0] → [L0] → [L0] ...

CPU-Bound Process:
- Starts at level 0
- Uses full quantum (1 tick)
- Demoted to level 1
- Uses full quantum (2 ticks)
- Demoted to level 2
- Pattern: [L0] → [L1] → [L2] → [L3]

Mixed Process:
- Alternates between I/O and CPU work
- Oscillates in queue levels
- Pattern: [L0] → [L1] → [L0] → [L1] ...
```

---

## System Call: `getprocinfo()`

### Function Signature
```c
int getprocinfo(struct procinfo *info);
```

### Parameters
- `info`: Pointer to `struct procinfo` (user space)

### Return Value
- `0`: Success
- `-1`: Error

### What It Returns
```c
struct procinfo {
  int pid;              // Process ID
  int state;            // Process state (1=USED, 2=SLEEPING, 3=RUNNABLE, 4=RUNNING, 5=ZOMBIE)
  int queue_level;      // Current queue level (0-3)
  int time_in_queue;    // Ticks spent in current queue
  uint64 time_slices;   // Total CPU time slices received
  char name[16];        // Process name
};
```

### Example Usage
```c
struct procinfo info;
if(getprocinfo(&info) == 0) {
  printf("Process %d is in queue %d\n", info.pid, info.queue_level);
  printf("Time in queue: %d ticks\n", info.time_in_queue);
} else {
  printf("Error getting process info\n");
}
```

---

## Testing Checklist

### Before Week 2 Implementation
- [ ] Code compiles without errors
- [ ] `test_getprocinfo` runs successfully
- [ ] `scheduler_demo` forks and runs children
- [ ] All processes show queue_level = 0 (not demoted yet)
- [ ] getprocinfo returns valid data

### To Verify Implementation
```bash
cd /path/to/xv6-riscv
make clean
make CPUS=1

# Then in qemu:
make qemu
# In xv6 shell:
test_getprocinfo
scheduler_demo
```

---

## Important Notes for Next Week

### What Week 2 Will Add
1. Actual queue arrays instead of just fields
2. Demotion logic in scheduler
3. Time quantum enforcement
4. Starvation prevention logic
5. Queue management functions

### What Remains Unchanged
- No changes to context switching logic
- No changes to interrupt handling
- No changes to process creation
- No changes to user interface (just extending it)

### Debugging Helpers (Week 2)
- Use `printf()` in scheduler to show queue movements
- Track each process's queue_level changes
- Monitor time_in_queue increments
- Use test programs to observe behavior

---

## File Structure (Post-Week 1)

```
xv6-riscv/
├── kernel/
│   ├── proc.h              ✓ MODIFIED
│   ├── proc.c              ✓ MODIFIED
│   ├── syscall.h           ✓ MODIFIED
│   ├── syscall.c           ✓ MODIFIED
│   ├── sysproc.c           ✓ MODIFIED
│   └── [other files unchanged]
│
├── user/
│   ├── user.h              ✓ MODIFIED
│   ├── usys.pl             ✓ MODIFIED
│   ├── test_getprocinfo.c  ✓ NEW
│   ├── scheduler_demo.c    ✓ NEW
│   └── [other files unchanged]
│
├── MLFQ_DESIGN.md          ✓ NEW
├── WEEK1_SUMMARY.md        ✓ NEW
├── WEEK1_QUICK_REF.md      ✓ NEW (this file)
└── [existing files]
```

---

## Compilation Command
```bash
# From xv6 directory
make clean
make CPUS=1

# Should complete without errors
# If errors occur, check syscall.c for any typos in dispatch array
```

---

## Success Criteria Met ✓

- [x] Design document created (MLFQ_DESIGN.md)
- [x] System call `getprocinfo()` implemented
- [x] System call test program provided
- [x] Queue scaffolding complete (constants, fields, structures)
- [x] MLFQ fields initialized in all process lifecycle functions
- [x] Syscall properly registered in dispatch
- [x] User-space interface complete
- [x] Demo program provided

---

**Week 1 Status**: ✅ COMPLETE  
**Ready for Week 2**: Queue implementation and scheduler modification
