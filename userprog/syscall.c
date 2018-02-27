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

/* Include statements */
#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/shutdown.h"
#include "userprog/process.h"

#define MAX_ARGS 3

/* Function definitions */
static void syscall_handler (struct intr_frame *f);
void get_arg (struct intr_frame *f, int *argv, int arg_num);
void is_valid_string (const void * str);
int get_kernel_ptr (const void *vaddr);
void is_valid_ptr (const void *vaddr);

/* 
syscall_init void function will initialize lock and intr_register_int to set up system calls
Input: none
Output: none
*/
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall"); 
}

/* 
syscall_handler void function will take in a struct frame for stack and call the appropriate system calls. Switch cases are used to call the correct system call.
Input: struct intr_frame
Output: none
*/
static void
syscall_handler (struct intr_frame *f) 
{
  int argv[MAX_ARGS];
  int esp = get_kernel_ptr ((const void *) f->esp);

  switch (*(int *) esp) 
    {
      case SYS_HALT:
      {
        halt ();
        break;
      }
      case SYS_EXIT:
      {
        get_arg (f, &argv[0], 1);
        exit ((int) argv[0]);
        break;
      }
      case SYS_EXEC:
      {
        get_arg (f, &argv[0], 1);
        is_valid_string ((const void *) argv[0]);
        argv[0] = get_kernel_ptr ((const void *) argv[0]);
        f->eax = exec ((const char *) argv[0]);
        break;
      }
      case SYS_WAIT:
      {
        get_arg (f, &argv[0], 1);
        f->eax = wait ((pid_t) argv[0]);
        break;
      }
      case SYS_CREATE:
      {
        get_arg (f, &argv[0], 2);
        is_valid_string ((const void *) argv[0]);
        argv[0] = get_kernel_ptr ((const void *) argv[0]);
        f->eax = create ((const char *) argv[0], (unsigned) argv[1]);
        break;
      }
      case SYS_REMOVE:
      {
        get_arg (f, &argv[0], 1);
        is_valid_string ((const void *) argv[0]);
        argv[0] = get_kernel_ptr ((const void *) argv[0]);
        f->eax = remove ((const char *) argv[0]);
        break;
      }
      case SYS_OPEN:
      {
        get_arg (f, &argv[0], 1);
        is_valid_string ((const void *) argv[0]);
        argv[0] = get_kernel_ptr ((const void *) argv[0]);
        f->eax = open ((const char *) argv[0]);
        break;
      }
      case SYS_FILESIZE:
      {
        get_arg (f, &argv[0], 1);
        f->eax = filesize ((int) argv[0]);
        break;
      }
      case SYS_READ:
      {
        get_arg (f, &argv[0], 3);
        is_valid_buffer ((const void *) argv[1], (unsigned) argv[2]);
        argv[1] = get_kernel_ptr ((const void *) argv[1]);
        f->eax = read ((int) argv[0], (void *) argv[1], (unsigned) argv[2]);
        break;
      }
     case SYS_WRITE:
      {
        get_arg (f, &argv[0], 3);
        is_valid_buffer ((const void *) argv[1], (unsigned) argv[2]);
        argv[1] = get_kernel_ptr ((const void *) argv[1]);
        f->eax = write ((int) argv[0], (const void *) argv[1], (unsigned) argv[2]); 
        break;
      }
      case SYS_SEEK:
      {
        get_arg (f, &argv[0], 2);
        seek ((int) argv[0], (unsigned) argv[1]);
        break;
      }
      case SYS_TELL:
      {
        get_arg (f, &argv[0], 1);
        f->eax = tell ((int) argv[0]);
        break;
      }
      case SYS_CLOSE:
      {
        get_arg (f, &argv[0], 1);
        close ((int) argv[0]);
        break;
      }
      case SYS_CHDIR:
      {
        get_arg (f, &argv[0], 1);
        is_valid_string ((const void *) argv[0]);
        argv[0] = get_kernel_ptr ((const void *) argv[0]);
        f->eax = chdir ((const char *) argv[0]);
        break;
      }
      case SYS_MKDIR:
      {
        get_arg (f, &argv[0], 1);
        is_valid_string ((const void *) argv[0]);
        argv[0] = get_kernel_ptr ((const void *) argv[0]);
        f->eax = mkdir ((const char *) argv[0]);
        break;
      }
      case SYS_READDIR:
      {
        get_arg (f, &argv[0], 2);
        is_valid_string ((const void *) argv[1]);
        argv[1] = get_kernel_ptr ((const void *) argv[1]);
        f->eax = readdir (argv[0], (char *) argv[1]);
        break;
      }
      case SYS_ISDIR:
      {
        get_arg (f, &argv[0], 1);
        f->eax = isdir (argv[0]);
        break;
      }
      case SYS_INUMBER:
      {
        get_arg (f, &argv[0], 1);
        f->eax = inumber (argv[0]);
        break;
      }
      default:
      {
        exit (ERROR);
      }
    }
}

/* 
Halt void function is a system call to teriminate pintos
Input: none
Output: none
*/
void
halt ()
{
  shutdown_power_off ();
}

/* 
Exit void function is a system call that takes an integer status and terminates the thread.
Input: integer status
Output: none
*/
void
exit (int status)
{
  struct thread *cur = thread_current ();
  if (thread_exist(cur->parent) && cur->child_proc) 
    {
      cur->child_proc->exit_status = status;
    }
  thread_exit ();
}

/* 
Exec pid_t function is a system call that takes in a char pointer cmd_line and calls the process_execute function. This function also passes the cmd_line argument to processs execute and returns with its value.
Input: const char pointer cmd_line
Output: none
*/
pid_t
exec (const char *cmd_line)
{
  return process_execute (cmd_line); 
}

/* 
wait int function is a system call that takes in a pid_t pid and returns the result of waiting. The value should be either an error value or the PID.
Input: pid_t pid
Output: integer value result
*/
int
wait (pid_t pid)
{  
  int result = process_wait (pid); 
  return result;
}

/* 
Create boolean function is a system call that takes in two arguments, const char pointer file and unsigned initial_size to create a file and returns True or False (creation success or failure).
Input: const char pointer file and unsigned initial_size
Output: boolean value True or False
*/
bool
create (const char *file, unsigned initial_size)
{ 
  bool result = filesys_create (file, initial_size, false);
  return result;
}

/* 
Remove boolean function is a system call that takes in a const char pointer file and removes the file. This function will return a boolean value True or False if the file was successfuly removed.
Input: const char pointer file
Output: boolean value True or False
*/
bool
remove (const char *file)
{
  bool result = filesys_remove (file);
  return result;
}

/* 
Open int function is a system call that takes a const char pointer file name and adds it to the list. If the file failed to open then immediately return an error.
Input: const char pointer file
Output: integer value error or result
*/
int
open (const char *file)
{
  struct file *f = filesys_open (file);
  if (!f)
    {
      return ERROR;
    }
  int result = ERROR;
  struct inode *inode = file_get_inode (f);
  if (inode->data.sub_dir)
    result = add_dir ((struct dir *) f);
  else
    result = add_file (f);
  return result;
}

/* 
Filesize int function is a system call that takes in a integer file descriptor and retrieves the file and returns the length (in bytes) of the file. If file descriptor is NULL, then return an error otherwise return the size of the file.
Input: integer value file descriptor
Output: integer value error or size
*/
int
filesize (int fd)
{
  struct file_process *fp = get_file(fd);
  if (!fp) 
    {
      return ERROR;
    }
  int size = file_length (fp->file);
  return size;
}

/* 
Read int function is a system call that takes in three arguments, integer value file descriptor, void pointer buffer, and an unsigned size, to read size bytes from the open file into a buffer. Returns number of bytes read or an error.
Input: integer value file descriptor, void *buffer, and an unsigned size
Output: integer value error or result
*/
int
read (int fd, void *buffer, unsigned size)
{
  if (fd == STDIN_FILENO) 
    {
      int counter;
      uint8_t *temp_buffer = (uint8_t *) buffer;
      for(counter = 0; counter < size; counter++) 
        {
          temp_buffer[counter] = input_getc ();
        }
      return size;
    }
  int result = ERROR;
  struct file_process *fp = get_file (fd);
  if (fp)
    result = file_read (fp->file, buffer, size);
  return result;
}

/* 
Write int function is a system call that takes in three arguments, int file descriptor, const void pointer buffer, and unsigned size, to write size bytes from buffer to an open file, fd. Returns the number of bytes actually written or error if there is an issue.
Input: integer value file descriptor, const void pointer buffer, and unsigned size
Output: integer value result or error
*/
int
write (int fd, const void *buffer, unsigned size)
{
  if (fd == STDOUT_FILENO) 
    {
      putbuf(buffer, size);
      return size;
    }
  int result = ERROR;
  struct file_process *fp = get_file (fd);
  if (!fp)
    return ERROR;
  if (fp->is_dir)
    return ERROR;
  if (fp)
    result = file_write (fp->file, buffer, size);
  return result;
}

/* 
Seek void function is a system call that takes 2 arguments, integer value file descriptor and unsigned position, to change the next byte to be read or written in the open file, fd, to position.
Input: integer value file descriptor and unsigned position
Output: none
*/
void
seek (int fd, unsigned position)
{
  struct file_process *fp = get_file (fd);
  if (fp->is_dir)
    return;
  if (fp)
    file_seek (fp->file, position);
  return;
}

/* 
Tell unsigned function is a system call takes in one argument, integer value file descriptor, to return the position of the next byte to be read or written in open file, fd. The returned value could be a value or an error.
Input: integer value file descriptor
Output: unsigned value result.
*/
unsigned
tell (int fd)
{
  int result = ERROR;
  struct file_process *fp = get_file (fd);
  if (fp->is_dir)
    return;
  if (fp)
    result = file_tell (fp->file);
}

/* 
Close void function is a system call that takes in an argument, integer value file descriptor, and closes the file descriptor, fd. This function will implicitly close all other file descriptors too.
Input: integer value file descriptor
Output: none
*/
void
close (int fd)
{
  struct file_process *fp = get_file (fd);
  if (!fp)
    return;
  if (!fp->is_dir)
    {
      file_close (fp->file);
      list_remove (&fp->file_elem);
      free(fp);
    }
  else
    {
      dir_close (fp->dir);
      list_remove (&fp->file_elem);
      free(fp);
    }
  return;
}

/*
chdir boolean function will take an argument, const char *dir_path, and call the filesys function for changing directories; returns true on success else false
Input: const char *dir_path
Output: true or false
*/
bool 
chdir (const char *dir_path)
{
  return filesys_chdir (dir_path);
}

/*
mkdir boolean function will take an argument, const char *dir_path, and call the filesys function for making a directory; returns true on success else false
Input: const char *dir_path
Output: true or false
*/
bool 
mkdir (const char *dir_path)
{
  return filesys_mkdir (dir_path);
}

/*
readdir boolean function will take two arguments, int fd and char *dir_path, and returns true upon successful reads into directory
Input: int fd and char *dir_path
Output: true or false
*/
bool 
readdir (int fd, char *dir_path)
{
  struct file_process *fp = get_file (fd);
  if (!fp)
    return false;
  if (!fp->is_dir)
    return false;
  return dir_readdir(fp->dir, dir_path);
}

/*
isdir boolean function will take an argument, int fd, and checks if the file descriptor is a directory
Input: int fd
Output: true or false
*/
bool 
isdir (int fd)
{
  struct file_process *fp = get_file (fd);
  if (!fp)
    return false;
  return fp->is_dir;
}

/*
inumber int function will take an argument, int fd, and get the inode from the file descriptor's directory
Input: int fd
Output: integer value
*/
int 
inumber (int fd)
{
  struct file_process *fp = get_file (fd);
  if (!fp)
    return ERROR;
  if (fp->is_dir)
    return inode_get_inumber (dir_get_inode (fp->dir));
  else
    return inode_get_inumber (file_get_inode (fp->file));
}

/* 
is_valid_ptr void function that takes an argument, const void pointer vaddr, checks to make sure that the pointer addresss is a valid pointer. If not, then exit with an error.
Input: constant void pointer vaadr
Output: none
*/
void
is_valid_ptr (const void *vaddr)
{
  if(!is_user_vaddr (vaddr))
    exit (ERROR);
}

/* 
get_kernel_ptr int function takes an argument, const void pointer vaddr, that will obtain the kernel pointer from the page
Input: const void pointer vaddr
Output: integer value error or Ptr
*/
int
get_kernel_ptr (const void *vaddr)
{
  struct thread *cur = thread_current();
  is_valid_ptr (vaddr);
  void *ptr = pagedir_get_page (cur->pagedir, vaddr);
  if (!ptr)
    exit (ERROR);
  return (int) ptr;
}

/* 
get_arg void function takes three arguments, struct intr_frame pointer f, integer value pointer argv, and integer value arg_num, to get the arguments on the stack.
Input: struct intr_frame *f, integer value *argv, integer value arg_num
Output: none
*/
void
get_arg (struct intr_frame *f, int *argv, int arg_num)
{
  int *ptr;
  int counter;
  for (counter = 0; counter < arg_num; counter++) 
    {
      ptr = (int *) f->esp + counter + 1;
      is_valid_ptr ((const void*) ptr);
      argv[counter] = *ptr;
    } 
}

/* 
is_valid_string void function takes in an argument, const void pointer str, that will check if the string is valid by checking every byte.
Input: const void pointer str
Output: none
*/
void
is_valid_string (const void *str)
{
  while (*(char *) get_kernel_ptr (str) != 0)
    str = (char *) str + 1;
}

/* 
is_valid_buffer void function takes in two arguments, const void pointer buffer and an unsigned size, to check the buffer for accuracy.
Input: const void pointer buffer, unsigned size
Output: none
*/
void
is_valid_buffer (const void *buffer, unsigned size)
{
  char *temp_buffer = (char *) buffer;
  int counter;
  for (counter = 0; counter < size; counter++)
    {
      is_valid_ptr ((const void*) temp_buffer);
      temp_buffer++;
    }
}

/* 
add_file int function takes in an argument, struct file pointer f, that will put the file into the list. Space is allocated for the file with malloc in this function as well.
Input: struct file pointer f
Output: integer value file descriptor
*/
int
add_file (struct file *f)
{
  struct thread *cur = thread_current ();
  struct file_process *fp = malloc (sizeof(struct file_process));
  if (!fp) 
    return ERROR;
  fp->file = f;
  fp->fd = cur->fd++;
  fp->is_dir = false;
  list_push_back (&cur->file_list, &fp->file_elem);
  return fp->fd;
}

/* 
add_dir int function takes in an argument, struct dir pointer f, that will put the file into the list. Space is allocated for the file with malloc in this function as well.
Input: struct dir pointer f
Output: integer value file descriptor
*/
int
add_dir (struct dir *dir)
{
  struct thread *cur = thread_current ();
  struct file_process *fp = malloc (sizeof(struct file_process));
  if (!fp) 
    return ERROR;
  fp->dir = dir;
  fp->fd = cur->fd++;
  fp->is_dir = true;
  list_push_back (&cur->file_list, &fp->file_elem);
  return fp->fd;
}

/* 
file_process struct function takes in an argument, integer value cur_fd, to get needed file from a list. The list is searched through for the file and is returned if found.
Input: integer value cur_fd
Output: NULL or struct file_process fp
*/
struct
file_process *get_file (int cur_fd)
{
  struct thread *cur = thread_current ();
  struct file_process *fp;
  struct list_elem *e;
  for (e = list_begin (&cur->file_list); e != list_end (&cur->file_list); e = list_next (e)) 
    {
      fp = list_entry (e, struct file_process, file_elem);
      if (fp->fd == cur_fd) 
        return fp;
    }
  return NULL;
}

/* 
close void function to iterate through each item in the list and then closes all old files in list and frees memory in the same process because they were each allocated earlier.
Input: none
Output: none
*/
void
close_all_file (void)
{
  struct thread *cur = thread_current ();
  struct list_elem *e = list_begin (&cur->file_list);
  while (e != list_end (&cur->file_list)) 
    {
      struct file_process *fp = list_entry (e, struct file_process, file_elem);
      e = list_next (e);
      close (fp->fd);
    }
  return;
}