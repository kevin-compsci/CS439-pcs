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

#ifndef VM_SWAP_H
#define VM_SWAP_H

/* Include and define definitions */
#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "bitmap.h"

#define SWAP_FREE 0
#define SECTOR_SIZE (PGSIZE / BLOCK_SECTOR_SIZE)

struct lock slock;				/* Lock for the swap block */
struct block *sblock;				/* Block for swap */
struct bitmap *sblock_map;			/* Block mapping */

/* Function Definitions */
void swap_init ();				/* Initialize the swap structure */
void swap_into (void *frame, size_t index);	/* Get swapped block into table */
size_t swap_away (void *frame);			/* Get target block for swap */

#endif
