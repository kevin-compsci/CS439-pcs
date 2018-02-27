/*
################
TEAM INFO
################
Name1: Sangwon Lee
EID1: sl34645
CS Login: bearbox
Email: fhqksl@hotmail.com
Unique Number: 51100

Names2: Kevin Nguyen
EID2: kdn433
CS Login: kxnguyen
Email: kxnguyen60@gmail.com
Unique Number: 51075

Slip days used: 0

################
*/

#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

/* Include statements */
#include <debug.h>
#include <list.h>
#include <stdint.h>
#include <threads/synch.h>

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */
#define INIT_STATUS 0
#define NO_PARENT -1
#define INIT_FD 2
#define NOT_LOADED 0
#define LOAD_SUCCESS 1
#define LOAD_FAIL -1

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion.  (So don't add elements below 
   THREAD_MAGIC.)
*/
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */

/* Struct to maintain the list of threads that will donate */
struct donor
  {
    struct list donor_list;
  };

/* Struct to maintain a list of child processes */
struct child
  {
    struct list child_list;
  };

/* Struct to maintain a list of files */
struct list_file
  {
    struct list file_list;
  };

/* Struct to hold set of data within the thread */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    int original_priority;              /* Original priority that this thread was holding */
    struct donor donor;                 /* Threads that donate for another */
    struct lock *try_lock;              /* Tries to do things based on if lock is being held or not */
    struct list_elem allelem;           /* List element for all threads list. */
    struct list_elem donor_elem;        /* list of elements that will donate to another */
    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    int fd;                             /* File Descriptor */
    int load_status;                    /* Load Status */
    char *file_name;                    /* file name saved */
    tid_t parent;                       /* tid of its parent */
    struct child_process *child_proc;   /* child process */
    struct child child_list;            /* Child process list */
    struct list_file file_list;         /* file list */
    struct semaphore sema_wait;	        /* semaphore for waiting thread */
    struct semaphore sema_load;         /* semaphore for loading and to prevent race conditions */
    struct file *executable;            /* struct file pointer to an executable */
    uint32_t *pagedir;                  /* Page directory. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
    int64_t timeChecker;                /* Checks the wake up time */
    struct semaphore sema_p;            /* Semaphore struct for thread */
    struct list_elem timer_elem;        /* shared between thread.c and timer.c */
    struct dir *cur_dir;                /* Current working directory */
  };

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

/* Function Definitions */
void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

void check_priority (void);
bool comparePriority (struct list_elem *, struct list_elem *, void *);
bool compare_timer (struct list_elem *, struct list_elem *, void *);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);
struct thread *get_thread(tid_t);
bool thread_exist(tid_t);
int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

#endif /* threads/thread.h */