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

#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */

/* Block device that contains the file system. */
struct block *fs_device;

/* Function Definitions */
void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size, bool sub_dir);
struct file *filesys_open (const char *name);
bool filesys_remove (const char *name);
bool filesys_chdir (const char *dir_path);
bool filesys_readdir (int fd, const char *dir_path);
bool filesys_isdir (int fd);
int filesys_inumber (int fd);
struct dir *get_dir (const char *dir_path);
char *get_name (const char *dir_path);

#endif /* filesys/filesys.h */