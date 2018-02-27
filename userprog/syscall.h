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

#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#define MAX_FILE 128

/* Include statements */
#include "threads/thread.h"
#include "threads/synch.h"
#include "userprog/process.h"

/* file process description */
struct file_process
  {
    struct file *file;
    struct dir *dir;
    int fd;
    bool is_dir;
    struct list_elem file_elem;
  };

/* Function definitions below for system calls */
void syscall_init (void);
void halt (void);
void exit (int status);
pid_t exec (const char *cmd_line);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
int add_file (struct file *f);
struct file_process *get_file (int cur_fd);
void close_all_file (void);
bool chdir (const char *dir_path);
bool mkdir (const char *dir_path);
bool readdir (int fd, char *dir_path);
bool isdir (int fd);
int inumber (int fd);

#endif /* userprog/syscall.h */