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

#ifndef VM_FRAME_H
#define VM_FRAME_H

/* Include and Define definitions */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "vm/page.h"
#include <list.h>

struct list frame_table;	/* list of frame entries */
struct lock frame_lock;		/* lock for frame */

/* Entry of the frame */
struct frame_table_entry 
  {
    tid_t tid;
    void *frame;
    void *user_vaddr;
    struct spage_entry *spe;
    struct list_elem elem;
  };

/* Function definitions */
void frame_table_init (void);					/* Initialize frame table */
void add_frame_table (void *frame, void *user_vaddr);		/* Add frame to frame table */
void remove_frame_table (void *frame);				/* Remove and free frame from frame table */
void *frame_evic (enum palloc_flags indicator);			/* Select frame for eviction */
void *frame_alloc (void *user_vaddr, enum palloc_flags size);	/* Alloc. size, frame table */

#endif
