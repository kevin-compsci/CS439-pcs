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

#ifndef VM_PAGE_H
#define VM_PAGE_H

/* Include and Define definitions */
#include <hash.h>
#include "vm/frame.h"

#define FILE 1
#define SWAP 2
#define STACK_CHECK 32
#define STACK_SIZE (1 << 23)  //8 MB

/* Supplemental page table entry */
struct spage_entry 
  {
    void *user_vaddr;
    bool write_bit;
    bool load_bit;
    bool pin_bit;
    uint8_t type;
    size_t offset;    
    size_t read_bytes;
    size_t zero_bytes;
    size_t swap_index;
    struct file *file;
    struct hash_elem elem;
  };

/* Function definitions */
void spage_table_init (struct hash *spe);											/* Initialize page table */
void spage_table_remove ();													/* Remove and free entry from page table */
bool spage_load (struct spage_entry *spe);											/* load for page table */
bool spage_load_file (struct spage_entry *spe);											/* load file for page table */
bool spage_load_swap (struct spage_entry *spe);											/* For swapping pages */
bool spage_table_add (struct file *file, int32_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable);	/* Add entry to page table */
bool stack_growth (void *user_vaddr);												/* Grow the stack size when needed */

struct spage_entry *get_spage_entry (void *user_vaddr);										/* Get an entry from the page table */

#endif
