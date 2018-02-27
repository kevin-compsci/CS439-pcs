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

#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H
#include <list.h>
#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"

struct bitmap;
#define INODE_MAGIC 0x494e4f44
#define INDEX_BLOCK_SIZE 128
#define DIRECT_BLOCK_SIZE 12
#define FIRST_LEV_INDEX 10
#define SECOND_LEV_INDEX 11
#define ERROR -1

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    uint32_t unused[109];               /* Not used. */
    block_sector_t ptr[DIRECT_BLOCK_SIZE];
    block_sector_t parent;
    size_t direct_level_index;
    size_t first_level_index;
    size_t second_level_index;
    bool sub_dir;
  };

/* In-memory inode. */
struct inode 
  {
    int open_cnt;                       /* Number of openers. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    off_t length;                       /* Inode length */
    bool removed;                       /* True if deleted, false otherwise. */
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    struct inode_disk data;             /* Inode content. */
    struct lock *lock;                  /* Inode lock for synchronization */
  };

/* Function definitions */
void inode_init (void);
bool inode_create (block_sector_t, off_t, bool sub_dir);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);
void inode_lock_acquire (struct inode *);
void inode_lock_release (struct inode *);
#endif /* filesys/inode.h */
