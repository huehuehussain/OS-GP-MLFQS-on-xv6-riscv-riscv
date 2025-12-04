#define SBRK_ERROR ((char *)-1)

struct stat;

// MLFQ Scheduler structures
struct procinfo {
  int pid;
  int state;
  int queue_level;
  int time_in_queue;
  uint64 time_slices;
  char name[16];
};

struct mlfq_stats {
  uint64 total_schedules;
  uint64 total_boosts;
  uint64 total_demotions;
  uint64 level_queue_count[4];
  uint64 level_schedules[4];
};

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(const char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sys_sbrk(int,int);
int pause(int);
int uptime(void);
int getprocinfo(struct procinfo*);
int boostproc(int);  // Week 3: Manual priority boost
int getschedulerstats(struct mlfq_stats*);  // Week 3: Get scheduler statistics

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
int atoi(const char*);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
char* sbrk(int);
char* sbrklazy(int);

// printf.c
void fprintf(int, const char*, ...) __attribute__ ((format (printf, 2, 3)));
void printf(const char*, ...) __attribute__ ((format (printf, 1, 2)));

// umalloc.c
void* malloc(uint);
void free(void*);
