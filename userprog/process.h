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

#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H
#define ERROR -1
#define INIT_EXIT_STATUS -1
#define ALL_CLOSE -2
#define MAX_ARGS_SIZE 4096

/* Include statements */
#include "threads/thread.h"

/* Typedef Definitions */
typedef int pid_t;

/* Function Definitions */
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
void store_stack (void *, void **, char **, size_t);
struct child_process *add_child_proc (tid_t tid);
struct child_process *get_child_proc (tid_t tid);
void remove_child_proc (struct child_process *child);
void free_child_proc (void);

/* Struct for the lock filesys */
struct lock lock_filesys;

/* Struct for the child process */
struct child_process
  {
    pid_t pid;
    bool exit;
    bool wait;
    int exit_status;
    struct list_elem child_elem;
    struct semaphore sema_zombi;
  };

#endif /* userprog/process.h */