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
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h" 
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "filesys/file.h"

/*
frame_table_init void function will initialize the frame table and a lock
Input: none (void)
Output: none
*/
void 
frame_table_init (void)
{
  lock_init (&frame_lock);
  list_init (&frame_table);
}

/*
frame_table_alloc void pointer function will take 2 arguments, void *user_vaddr and enum palloc_flags, and the Frame table will be allocated as needed. Evict, if frame is not successful.
Input: void *user_vaddr, enum palloc_flags size
Output: none
*/
void 
*frame_table_alloc (void *user_vaddr, enum palloc_flags size)
{
  lock_acquire (&frame_lock);  
  if ((size <= 0) || (PAL_USER <= 0))  //error check if we can allocate a page or not 
    return NULL;

  void *frame = palloc_get_page (size);  //get frame
  if (frame)
    add_frame_table (frame, user_vaddr);  //add frame to frame table
  else
    { 
      if (!frame)
        {
          frame = frame_evic (size);  //eviction to get a frame
        }
      if (!frame)
        PANIC ("ERROR");
      add_frame_table (frame, user_vaddr);
    }
  lock_release (&frame_lock); 
  return frame;
}

/*
add_frame_table void function will take 2 arguments, void *frame and void *user_vaddr, and new info will be added to the Frame Table. Locks are used to avoid race conditions when the frame table is being updated.
Input: void *frame, void *user_vaddr
Output: none
*/
void
add_frame_table (void *frame, void *user_vaddr)
{

  struct frame_table_entry *fte = malloc (sizeof (struct frame_table_entry));
  fte->frame = frame;
  fte->user_vaddr = user_vaddr;
  fte->tid = thread_current ()->tid;
  list_push_back (&frame_table, &fte->elem);
}

/*
remove_frame_table void function will take in an argument, void *frame, and the Frame is removed from Frame Table and free'd of its resources after searching through the frame table for a target.
Input: void *frame
Output: none
*/
void 
remove_frame_table (void *frame)
{       
  lock_acquire (&frame_lock);
  struct list_elem *e = list_begin (&frame_table);
  while (e != list_end (&frame_table))
    {
      struct frame_table_entry *fte = list_entry (e, struct frame_table_entry, elem);
      if (fte->frame == frame) 
        {

          list_remove (e);      
          palloc_free_page (frame);
	  free (fte);
          lock_release (&frame_lock);
          return;
        }
      e = list_next (e);  
    }
  lock_release (&frame_lock);
  return;
}

/*
frame_evic void pointer function will take an argument, enum palloc_flags indicator, and search through the frame table to evict an entry based on the clock replacement policy. The searching in the list will be circular and locks are used to avoid interference.
Input: enum palloc_flags indicator
Output: a pointer
*/
void 
*frame_evic (enum palloc_flags indicator)
{

  struct list_elem *e = list_begin (&frame_table);
  struct frame_table_entry *fte;
 
  while (true)
    {
      fte = list_entry (e, struct frame_table_entry, elem);
      struct spage_entry *spe = get_spage_entry (pg_round_down (fte->user_vaddr));

      if (spe && !spe->pin_bit)
        {
          struct thread *t = get_thread (fte -> tid);
          if (!pagedir_is_accessed (t->pagedir, spe->user_vaddr))
            {	  		     
	      bool is_dirty = pagedir_is_dirty (t->pagedir, spe->user_vaddr);  	 
              /* This conditional is only if we need to swap */                        
              if (spe->type == SWAP || is_dirty) 
              {
                 spe->type = SWAP;
                 spe->swap_index = swap_away (fte->frame);
              }

              /* remove, clear, and free resources below */
	      list_remove (&fte->elem);	     
              pagedir_clear_page (t->pagedir, spe->user_vaddr);
	      palloc_free_page (fte->frame);
	      free (fte);
              spe->load_bit = false;
              return palloc_get_page (indicator);
            }
          else 
            pagedir_set_accessed (t->pagedir, spe->user_vaddr, false);
        }
      /* Keeps iterations on list circular */
      e = list_next (e);
      if (e == list_end (&frame_table))
        e = list_begin (&frame_table);
    }
    return NULL;
}
