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

/* Include definitions */
#include "swap.h"

/*
Swap_init void function will initialize the structure for swapping. No inputs or outputs.
Input: none
Output: none
*/
void
swap_init ()
{
  lock_init (&slock);  

  sblock = block_get_role (BLOCK_SWAP);  //Error Check: get block, if unable then return
  if (!sblock)
    return;
  
  sblock_map = bitmap_create (block_size (sblock) / SECTOR_SIZE);  //Error Check: create map, else return
  if (!sblock_map)
    return;

  bitmap_set_all (sblock_map, SWAP_FREE);
}

/*
Swap_into void function will take 2 inputs a pointer frame and an index to swap a page back into a position. 
Input: void *frame, size_t index
Output: none
*/
void
swap_into (void *frame, size_t index)
{
  /* Variable Declarations */
  size_t counter = 0;    
  lock_acquire(&slock);
  if (!sblock_map || !sblock || bitmap_test(sblock_map, index) == SWAP_FREE)  //Error Checking
    {
      lock_release(&slock);
      return;  //NOTE: may need to indicate PANIC!
    }

  while (counter < SECTOR_SIZE)  //Read each sector from block to buffer
    {
      block_read (sblock, index * SECTOR_SIZE + counter, (uint8_t *) frame + counter * BLOCK_SECTOR_SIZE);
      counter++;
    }
    bitmap_flip (sblock_map, index);
  lock_release(&slock);
}

/*
Swap_away size_t function will take an argument, void *frame. That frame will be swapped out.
Input: void *frame
Output: size_t result or ERROR
*/
size_t
swap_away (void *frame)
{
  /* Variable Declarations */
  size_t swap_index;
  size_t counter = 0;
  if (!sblock_map || !sblock)  //Error Checking
    {
      return;  //NOTE: may need to indicate PANIC!
    }
  lock_acquire (&slock);
  swap_index = bitmap_scan_and_flip (sblock_map, 0, 1, SWAP_FREE);
  if (swap_index == BITMAP_ERROR)  //Error Checking
    {
      lock_release (&slock);
      return;  //NOTE: may  need to inidcate PANIC!
    }

   while (counter < SECTOR_SIZE)  //Write each sector to block from buffer
    {
      block_write (sblock, swap_index * SECTOR_SIZE + counter, (uint8_t *) frame + counter * BLOCK_SECTOR_SIZE);
      counter++;
    }
      
  lock_release (&slock);

  return swap_index;
}
