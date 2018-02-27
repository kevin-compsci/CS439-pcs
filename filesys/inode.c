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

/* Include definitions */
#include "filesys/inode.h"
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* Struct to keep track of sector positions */
struct indexed_block
  {
    block_sector_t pointers[INDEX_BLOCK_SIZE];
  };

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* Function Definitions */
bool inode_allocate (struct inode_disk *disk_inode);
off_t inode_grow (off_t disk_length, struct inode *inode);
size_t first_indir_alloc (size_t sectors, struct inode *inode);
size_t second_indir_alloc (size_t sectors, struct inode *inode);
size_t second_indir_alloc_deep (size_t sectors, struct inode *inode, struct indexed_block *second_block);
void inode_deallocate (struct inode *disk_inode);
size_t first_level_dealloc (size_t size, struct inode *inode);
size_t second_level_dealloc (size_t size, struct inode *inode);
size_t second_level_dealloc_deep (size_t size, int index, struct indexed_block *second_block);

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
  off_t origin = pos;
  ASSERT (inode != NULL);
  /* Multilevel indexing algorithm below */
  if (pos < inode_length (inode))
    {
      pos = pos / BLOCK_SECTOR_SIZE;
      if (pos < FIRST_LEV_INDEX)  //Direct block
        return inode->data.ptr[pos];

      pos -= FIRST_LEV_INDEX;
      if (pos < INDEX_BLOCK_SIZE)  //First level indirection indexing blocks
        {
          struct indexed_block block;
          block_read (fs_device, inode->data.ptr[FIRST_LEV_INDEX], &block);
          return block.pointers[pos];
        }

      pos -= INDEX_BLOCK_SIZE;
      if (pos < INDEX_BLOCK_SIZE * INDEX_BLOCK_SIZE)  //Second Level indirection indexing blocks
        {
          off_t off = pos % INDEX_BLOCK_SIZE;
          pos = pos / INDEX_BLOCK_SIZE;

          struct indexed_block second_block;
          block_read (fs_device, inode->data.ptr[SECOND_LEV_INDEX], &second_block);
          struct indexed_block block;
          block_read (fs_device, second_block.pointers[pos], &block);
          return block.pointers[off];
        }
    }
  else
    return ERROR;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool sub_dir)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);
  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      disk_inode->sub_dir = sub_dir;
      disk_inode->parent = ROOT_DIR_SECTOR;
      if (inode_allocate (disk_inode))
        {
          block_write (fs_device, sector, disk_inode);
          success = true; 
        } 
      free (disk_inode);
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  lock_init (&inode->lock);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  block_read (fs_device, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk. (Does it?  Check code.)
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);	
          inode_deallocate (inode);  
        }
      else
        {
          struct inode_disk disk_inode = inode->data;
          memcpy (&disk_inode.ptr, &inode->data.ptr, sizeof (block_sector_t) * DIRECT_BLOCK_SIZE);
          block_write (fs_device, inode->sector, &disk_inode);
        }
      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  uint8_t *bounce = NULL;
  off_t bytes_read = 0;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (!bounce) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (!bounce)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);
  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;
  
  if (inode->deny_write_cnt)
    return 0;

  if (offset + size > inode_length(inode))
    {
      if (!inode->data.sub_dir)
        lock_acquire (&inode->lock);
      inode->data.length = inode_grow (offset + size, inode);
      if (!inode->data.sub_dir)
        lock_release (&inode->lock);
    }
 
   while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;
  
      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;
      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

/* Returns the length, in bytes, of INODE's data. */
bool
inode_allocate (struct inode_disk *disk_inode)
{
  struct inode inode;
  inode.data.direct_level_index = 0;
  inode.data.first_level_index = 0;
  inode.data.second_level_index = 0;
  inode.data.length = 0;
  inode_grow (disk_inode->length, &inode);
  disk_inode->direct_level_index = inode.data.direct_level_index;
  disk_inode->first_level_index = inode.data.first_level_index;
  disk_inode->second_level_index = inode.data.second_level_index;
  memcpy (&disk_inode->ptr, &inode.data.ptr, DIRECT_BLOCK_SIZE * sizeof (block_sector_t));
  return true;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_grow (off_t disk_length, struct inode *inode)
{
  /* local variable declarations */
  static char zeros[BLOCK_SECTOR_SIZE];
  size_t sectors = bytes_to_sectors (disk_length) - bytes_to_sectors (inode_length (inode));

  if (sectors == 0)  //Error check
    return disk_length;

  // allocate direct block first
  int index = inode->data.direct_level_index;
  for (; (index < FIRST_LEV_INDEX && sectors !=0); index++, sectors--)
    {
      free_map_allocate (1, &inode->data.ptr[index]);
      block_write (fs_device, inode->data.ptr[index], zeros);
    }
  inode->data.direct_level_index = index;
  if (!sectors)
    return disk_length;

  // 1st level indexed block
  if (inode->data.direct_level_index == FIRST_LEV_INDEX)
    {
      sectors = first_indir_alloc (sectors, inode);
      if (sectors == ERROR)
        return ERROR;
      else if (!sectors)
        return disk_length;
    }
  if (inode->data.direct_level_index == SECOND_LEV_INDEX)
    {
      sectors = second_indir_alloc (sectors, inode);
      if (sectors == ERROR)
        return ERROR;
    }
  // return value should be disk_length. sectors should be zero;
  return disk_length - sectors * BLOCK_SECTOR_SIZE;
}

/*
first_indir_alloc size_t function will take two arguments, size_t sectors and struct inode *inode, and allocate block from the first level indirection
Input: size_t sectors and struct inode *inode
Output: size_t value
*/
size_t
first_indir_alloc (size_t sectors, struct inode *inode)
{
  /* Local variable declaration */
  static char zeros[BLOCK_SECTOR_SIZE];
  struct indexed_block index_block;

  if (inode->data.first_level_index == 0)
    free_map_allocate (1, &inode->data.ptr[FIRST_LEV_INDEX]);
  else
    block_read (fs_device, inode->data.ptr[FIRST_LEV_INDEX], &index_block);

  int index = inode->data.first_level_index;
  for (; (index < INDEX_BLOCK_SIZE && sectors !=0); index++, sectors--)
    {
      free_map_allocate (1, &index_block.pointers[index]);
      block_write (fs_device, index_block.pointers[index], zeros);
    }
  inode->data.first_level_index = index;
  block_write (fs_device, inode->data.ptr[FIRST_LEV_INDEX], &index_block);
  
  if (inode->data.first_level_index == INDEX_BLOCK_SIZE)
    {
      inode->data.first_level_index = 0;
      inode->data.direct_level_index++;
    }
  return sectors;
}

/*
Second_indir_alloc size_t function will take two arguments, size_t sectors and struct inode *inode, and allocate second level indirections as needed
Input: size_t sectors and struct inode *inode
Output: size_t value
*/
size_t
second_indir_alloc (size_t sectors, struct inode *inode)
{
  /* Local variable declaration */
  struct indexed_block second_block;

  if (inode->data.first_level_index == 0 && inode->data.second_level_index == 0)
    free_map_allocate (1, &inode->data.ptr[SECOND_LEV_INDEX]);  
  else
    block_read (fs_device, inode->data.ptr[SECOND_LEV_INDEX], &second_block);
  
  while (inode->data.first_level_index < INDEX_BLOCK_SIZE)
    {
      sectors = second_indir_alloc_deep (sectors, inode, &second_block);
      if (sectors == 0)
        break;
    }
  block_write (fs_device, inode->data.ptr[SECOND_LEV_INDEX], &second_block);
  return sectors;
}

/*
second_indir_alloc_deep size_t function takes in three arguments, size_t sectors, struct inode *inode, struct indexed_block *second_block, and will allocate the inner second indirection block as needed
Input: size_t sectors, struct inode *inode, struct indexed_block *second_block
Output: size_t value
*/
size_t
second_indir_alloc_deep (size_t sectors, struct inode *inode, struct indexed_block *second_block)
{
  static char zeros[BLOCK_SECTOR_SIZE];
  struct indexed_block first_block;
  if (inode->data.second_level_index == 0)
    free_map_allocate (1, &second_block->pointers[inode->data.first_level_index]);
  else
    block_read (fs_device, second_block->pointers[inode->data.first_level_index], &first_block);
  int index = inode->data.second_level_index;

  for (; (index < INDEX_BLOCK_SIZE && sectors !=0); index++, sectors--)
    {
      free_map_allocate (1, &first_block.pointers[index]);
      block_write (fs_device, first_block.pointers[index], zeros);
    }
  inode->data.second_level_index = index;
  block_write (fs_device, second_block->pointers[inode->data.first_level_index], &first_block);

  if (inode->data.second_level_index == INDEX_BLOCK_SIZE)
    {
      inode->data.first_level_index++;
      inode->data.second_level_index = 0;
    }
  return sectors;
}

/*
inode_deallocate void function will take an argument, struct inode *inode, and deallocate any resources from an inode
Input: struct inode *inode
Output: none
*/
void
inode_deallocate (struct inode *inode)
{
  size_t size = bytes_to_sectors (inode_length (inode));
  int index = 0;

  if (size == 0)  //Error Checking; PANIC to trace possible bugs
    return;

  while (index < FIRST_LEV_INDEX)
    {
      free_map_release (inode->data.ptr[index], 1);
      index++;
      size--;
      if (size == 0)  //Error Checking; PANIC to trace possible bugs
        return;
    }
  size = first_level_dealloc (size, inode);
  free_map_release (inode->data.ptr[FIRST_LEV_INDEX], 1);

  if (size == 0)  //Error Checking; PANIC to trace possible bugs
    return;

  size = second_level_dealloc (size, inode);
  free_map_release (inode->data.ptr[SECOND_LEV_INDEX], 1);
  return;
}

/*
first_level_dealloc size_t function will take two arguments, size_t size and struct inode *inode, and deallocate first level indirections
Input: size_t size and struct inode *inode
Output: size_t value
*/
size_t
first_level_dealloc (size_t size, struct inode *inode)
{
  struct indexed_block block;
  block_read (fs_device, inode->data.ptr[FIRST_LEV_INDEX], &block);
  int index = 0;

  while (index < INDEX_BLOCK_SIZE)
    {
      free_map_release (block.pointers[index], 1);
      index++;
      size--;
      if (size == 0)  //Error Checking...PANIC?
        return size; 
    }
  return size;
}

/*
second_level_dealloc size_t function will take two arguments, size_t size and struct inode *inode, and deallocate second level indirections
Input: size_t size and struct inode *inode
Output: size_t value
*/
size_t
second_level_dealloc (size_t size, struct inode *inode)
{
  /* Local variable declarations */
  struct indexed_block second_block;
  block_read (fs_device, inode->data.ptr[SECOND_LEV_INDEX], &second_block);
  int index = 0;

  while (index < INDEX_BLOCK_SIZE)
    {
      size = second_level_dealloc_deep (size, index, &second_block);
      index++;
      if (size == 0)
        return size;
    }
}

/*
second_level_dealloc_deep size_t function will take three arguments, size_t size, int index, and struct indexed_block *second_block, and deallocate inner second level indirections
Input: size_t size, int index, and struct idnexed_block *second_block
Output: size_t value
*/
size_t
second_level_dealloc_deep (size_t size, int index, struct indexed_block *second_block)
{
  /* Local variable declarations */
  struct indexed_block first_block;
  block_read (fs_device, second_block->pointers[index], &first_block);
  int count = 0;

  while (count < INDEX_BLOCK_SIZE)
    {
      free_map_release (first_block.pointers[count], 1);
      size--;
      count++;
      if (size == 0)  //Error check
        return size;
    }
  return size;
}