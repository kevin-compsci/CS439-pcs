/*
################
TEAM INFO
################
Names1: Kevin Nguyen
EID1: kdn433
CS Login: kxnguyen
Email: kxnguyen60@gmail.com
Unique Number: 51075

Name2: Sangwon Lee
EID2: sl34645
CS Login: bearbox
Email: fhqksl@hotmail.com
Unique Number: 51100


Slip days used: 1

################
*/

/* Include and Define definitions */
#include <stdbool.h>
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/interrupt.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "filesys/file.h"

/*
Page_hash_func will take two inputs a const struct hash element and a void *aux that will obtain a hash value for the element.
Input: const struct hash_elem *h_elem, void *aux
Output: unsigned value
*/
unsigned
page_hash_func (const struct hash_elem *h_elem, void *aux UNUSED) 
{
  struct spage_entry *spe = hash_entry (h_elem, struct spage_entry, elem);
  return hash_bytes (&spe->user_vaddr, sizeof (spe->user_vaddr));
}

/*
Page_less_function is a boolean function that will take in 3 inputs, hash elements a and b, and an *aux. A value is obtained from the respective hash table and check to ensure that address is valid and the target element is less than.
Input: const struct hash_elem *a, const struct hash_elem *b, void *aux
Output: True or False
*/
bool
page_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  struct spage_entry *spe_a = hash_entry (a, struct spage_entry, elem);
  struct spage_entry *spe_b = hash_entry (b, struct spage_entry, elem);
  return spe_a->user_vaddr < spe_b->user_vaddr;
}

/*
Page_action_func is a void function that will take 2 inputs, const struct hash *elem and void *aux. This function gets an value from the Hash Table and check the load bit to free a page entry and/or a frame.
Input: const struct hash *elem, void *aux
Output: none
*/
void
page_action_func (const struct hash_elem *h_elem, void *aux UNUSED)
{
  struct spage_entry *spe = hash_entry (h_elem, struct spage_entry, elem);
  if (spe->load_bit) 
    {
      void *frame = pagedir_get_page (thread_current ()->pagedir, spe->user_vaddr);
      remove_frame_table (frame);
      pagedir_clear_page (thread_current ()->pagedir, spe->user_vaddr);
    }
  free (spe);
}

/*
spage_table_init void function will take an argument, struct hash *spe, so the Page table is initialized
Input: struct hash *spe
Output: none
*/
void
spage_table_init (struct hash *spe)
{
  hash_init (spe, page_hash_func, page_less_func, NULL);
}

/*
Spage_table_destroy void function will take an argument, struct hash *spe, and destroy the hash table for the process when it exits.
Input: struct hash *spe
Output: none
*/
void
spage_table_destroy (struct hash *spe)
{
  hash_destroy (spe, page_action_func);
}

/* 
*get_spage_entry struct function will take an argument, void *user_vaddr, to get an entry from the hash table.
Input: void *user_vaddr
Output: none
*/
struct spage_entry *get_spage_entry (void *user_vaddr)
{
  struct spage_entry spe;
  spe.user_vaddr = user_vaddr;
  struct hash_elem *h_elem = hash_find (&thread_current()->spe, &spe.elem);
  if (!h_elem) 
    return NULL;
  else
    return hash_entry (h_elem, struct spage_entry, elem);
}

/* 
spage_load boolean function will take an argument, struct spage_entry *spe, and set the load condition as needed.
Input: struct spage_entry *spe
Output: true or false
*/
bool 
spage_load (struct spage_entry *spe)
  {
    bool result = false;
    spe->pin_bit = true;
 
    if (spe->type == FILE)
      result = spage_load_file (spe);
    else if (spe->type == SWAP)
      result = spage_load_swap (spe);
      
    return result;
  }

/* 
spage_load_file boolean function will take an argument, struct spage_entry *spe, and set the load condition (if success) from the file. Locks are needed to avoid race conditions.
Input: struct spage_entry *spe
Output: true or false
*/
bool
spage_load_file (struct spage_entry *spe)
{
  enum palloc_flags flags = PAL_USER;
  if (spe->zero_bytes == PGSIZE)
    flags |= PAL_ZERO;
  uint8_t *kpage = frame_table_alloc (spe->user_vaddr, flags);   
  if (kpage == NULL)
    return false;
  if (spe->zero_bytes != PGSIZE)
    {
      lock_acquire (&lock_filesys);
      if (file_read_at (spe->file, kpage, spe->read_bytes, spe->offset) != (int) spe->read_bytes)
        {
          lock_release (&lock_filesys);
          remove_frame_table (kpage);
          return false; 
        }
      lock_release (&lock_filesys);
      memset (kpage + spe->read_bytes, 0, spe->zero_bytes);
    }
   /* Add the page to the process's address space. */
  if (!install_page (spe->user_vaddr, kpage, spe->write_bit)) 
    {
      remove_frame_table (kpage);
      return false; 
    }
  spe->load_bit = true;
  return true;	 
}

/* 
spage_load_swap boolean function will take in an argument, spage_entry *spe, to help swap between swap table and frame
Input: struct spage_entry *spe
Output: true or false
*/
bool
spage_load_swap (struct spage_entry *spe)
{
  uint8_t *frame = frame_table_alloc (spe->user_vaddr, PAL_USER);
  if (!frame)
    return false;
  else if (frame && !install_page (spe->user_vaddr, frame, spe->write_bit))
    {
      remove_frame_table (frame);
      return false;
    }
  swap_into (spe->user_vaddr, spe->swap_index); 
  spe->load_bit = true;
  return true;
}

/* 
spage_table_add boolean function will take in 6 arguments, struct file *file, int32_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable, and add an entry to the supp. page table. That new entry will have data set to it based on arguments.
Input: struct file *file, int32_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable
Output: true or false
*/
bool 
spage_table_add (struct file *file, int32_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  struct spage_entry *spe = malloc (sizeof (struct spage_entry));

  if (!spe)
    return false;
  /* Set new entry with data from arguments */
  spe->file = file;
  spe->offset = ofs;
  spe->user_vaddr = upage;
  spe->read_bytes = read_bytes;
  spe->zero_bytes = zero_bytes;
  spe->write_bit = writable;
  spe->type = FILE;
  spe->load_bit = false;
  spe->pin_bit = false; 
  if (!hash_insert (&thread_current ()->spe, &spe->elem))
    return true;
  else
    return false;
}

/* 
stack_growth boolean function will take an argument, void *user_vaddr, and allocate space for the supp. page table and indicate whether or not we need to grow the stack frame.
Input: void *user_vaddr
Output: true or false
*/
bool 
stack_growth (void *user_vaddr)
{
  struct spage_entry *spe = malloc (sizeof (struct spage_entry));

  if (!spe)
    return false;

  spe->user_vaddr = user_vaddr;
  spe->write_bit = true;
  spe->load_bit = true;
  spe->pin_bit = true;
  spe->type = SWAP;

  uint8_t *frame = frame_table_alloc (spe->user_vaddr, PAL_USER);  //Error Checking
  if (!frame)
    {
      free (spe);
      return false;
    }

  if (!install_page (spe->user_vaddr, frame, spe->write_bit))  //Error Checking
   {
     free (spe);
     remove_frame_table (frame);
     return false; 
   }    
  if (intr_context ())
    {
      spe->pin_bit = false;
    } 
  if (!hash_insert (&thread_current ()->spe, &spe->elem))  //Error Checking
    return true;
  else
    return false;
}
