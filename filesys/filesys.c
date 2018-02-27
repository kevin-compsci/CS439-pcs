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

#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"
#include "threads/malloc.h"

/* Partition that contains the file system. */
struct block *fs_device;
static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size, bool sub_dir) 
{
  block_sector_t inode_sector = 0;
  struct inode *inode = NULL;
  struct dir *dir = get_dir (name);
  char *file_name = get_name (name);
  bool success = false;
  int check = strcmp (file_name, ".") + strcmp (file_name, "..");
  if (check !=0)
    {
      success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector,initial_size, sub_dir)
                  && dir_add (dir, file_name, inode_sector));
    }
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);
  free (file_name);
  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  /* Local variable declarations */
  struct inode *inode = NULL;
  struct dir *dir;
  char *file_name;
  int length;

  if (strlen (name) == 0 || !name)  //Error Checking, PANIC if needed maybe bugs?
    return NULL;

  dir = get_dir (name);
  file_name = get_name (name);
  length = strlen (file_name);

  /* Descend into directory path and get directory name */
  if (dir)
    {
      if (strcmp (file_name, ".") == 0)
        {
          free (file_name);
          return dir;
        }
      else if (strcmp (file_name, "..") == 0)
        {
          inode = dir_get_parent (dir);
          if (!inode)
            {          
              free (file_name);
              return NULL;
            }
        }
      else if (dir_root (dir) && length == 0)
        {
          free (file_name);
          return dir;
        } 
     }
  /* close & free existing previous directory */
  if (dir || !inode)
    {
      dir_lookup (dir, file_name, &inode);
      dir_close (dir);
      free (file_name);
      if (!inode)
        return NULL;
    }

  if (inode->data.sub_dir)
    return dir_open (inode);

  return file_open (inode);
}

/* Deletes the file named NAME.

   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = get_dir (name);
  char *file_name = get_name (name);
  bool success = dir != NULL && dir_remove (dir, file_name);
  dir_close (dir);
  free (file_name);
  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

/*
filesys_chdir boolean function will take an argument, const char *name, and change the current working to a different one
Input: const char *name
Output: true or false
*/
bool 
filesys_chdir (const char *dir_path)
{
  /* Local variable declarations */
  struct inode *inode = NULL;
  struct dir *dir = get_dir (dir_path);
  char *file_name = get_name (dir_path);
  int length = strlen (file_name);

  /* based on dir_path, select the algorithm that is appropriate */
  if (dir != NULL)
    {
      if (strcmp (file_name, ".") == 0)
        {
          thread_current ()->cur_dir = dir;
          free (file_name);
          return true;
        }
      else if (strcmp (file_name, "..") == 0)
        {
          inode = dir_get_parent (dir);
          if (!inode)
            {          
              free (file_name);
              return false;
            }
        }
      else if ((dir_root (dir) && length <= 0))
        {
          thread_current ()->cur_dir = dir;
          free (file_name);
          return true;
        }
    }
  /* close & free existing previous directory */
  if (dir || !inode)
    {
      dir_lookup (dir, file_name, &inode);
      dir_close (dir);
      free (file_name);
      if (!inode)
        return NULL;
    }

  /* open new directory */
  dir = dir_open (inode);
  if (!dir)  //Error Checking, no directory then return false
    return false;

  dir_close (thread_current()->cur_dir);
  thread_current()->cur_dir = dir;
  return true;
}

/*
filesys_mkdir boolean function will take an argument, const char *dir_path, and makes a new directory
Input: const char *dir_path
Output: true or false
*/
bool
filesys_mkdir (const char *dir_path)
{
  /* Local variable declarations */
  block_sector_t inode_sector = 0;
  //struct inode *inode = NULL;
  struct dir *dir = get_dir (dir_path);
  char *file_name = get_name (dir_path);
  bool success = false;
  int check = strcmp (file_name, ".") + strcmp (file_name, "..");

  /* if check is > 0, then we have some file_name str to use */
  if (check)
    {
      success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, 0, true)
                  && dir_add (dir, file_name, inode_sector));
    }

  /* Free and close directory */
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  free (file_name);
  dir_close (dir);
  return success;
}

/*
*get_dir struct function will take an argument, const char *dir_path, and get the directory data from the indicated path.
Input: const char *dir_path
Output: struct dir
*/
struct dir *
get_dir (const char *dir_path)
{ 
  /* Local variable declaration */ 
  struct dir *dir;
  int length = strlen (dir_path);
  char *token_prev;
  char *token_curr;
  char *save_ptr;
  char temp[length + 1];
  memcpy (temp, dir_path, length + 1);

  /* Check if directory is root */
  if (temp[0] == '/' || !thread_current()->cur_dir)
    dir = dir_open_root ();
  else
    dir = dir_reopen (thread_current()->cur_dir);

  /* Loop through string tokens to get directory path */
  token_prev = strtok_r (temp, "/", &save_ptr);
  for (token_curr = strtok_r (NULL, "/", &save_ptr);
  token_curr != NULL;token_prev = token_curr,
      token_curr = strtok_r (NULL, "/", &save_ptr) )
    {
      struct inode *inode;
      if (strcmp (token_prev, ".") != 0)
        {
          if (strcmp (token_prev, "..") == 0)
            {
              inode = dir_get_parent (dir);
              if (!inode)         
                return NULL;  //Error check; PANIC if needed
            }
          else if (!dir_lookup (dir, token_prev, &inode))
            return NULL;

          if (inode && inode->data.sub_dir)
            {
              dir_close (dir);
              dir = dir_open (inode);
            }
          else
            {
              inode_close (inode);
              break;
            }
        }
    }
  return dir;
}

/*
*get_name char function will take an argument, const char *dir_path, and get the name of the directory from the indicated path.
Input: const char *dir_path
Output: character
*/
char *
get_name (const char *dir_path)
{
  /* Local variable declarations */
  int str_length = strlen (dir_path);
  char *token_prev = "";
  char *token_curr; 
  char *save_ptr; 
  char path[str_length + 1];
  memcpy (path, dir_path, str_length + 1);  
  token_curr= strtok_r (path, "/", &save_ptr);

  /* Get last token for the last directory path */
  for (; token_curr != NULL; token_prev = token_curr, token_curr = strtok_r (NULL, "/", &save_ptr));

  int file_length = strlen (token_prev) + 1;
  char *file_name = malloc (file_length);
  memcpy (file_name, token_prev, file_length);
  return file_name;
}