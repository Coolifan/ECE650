//
//  my_malloc.c
//  my_malloc
//
//  Created by Yifan Li on 1/21/18.
//  Copyright © 2018 Yifan Li. All rights reserved.
//
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include "my_malloc.h"

void *begin = NULL;

block_info *get_newblock(size_t size, block_info *lastblock) {
    block_info *newblock = sbrk(0);
    if (sbrk(size+BLOCK_INFO_SIZE) == (void *) -1) {
        fprintf(stderr, "sbrk failed\n");
        return NULL;
    }
    
    newblock->blockSize = size;
    newblock->next = NULL;
    newblock->prev = lastblock;
    
    if (lastblock) { // had some blocks before
        lastblock->next = newblock; //append the new block
    }
    newblock->isFree = 0;
    return newblock;
}

block_info *ff_find_free_region(size_t size, block_info **lastblock_ptr) {
    block_info * curr_block = begin;
    while ( (curr_block != NULL) && !(curr_block->isFree && curr_block->blockSize >= size) ) {
        *lastblock_ptr = curr_block;
        curr_block = curr_block->next;
    }
    return curr_block;
}

void split_block(block_info * b, size_t s) {
  block_info* remainder = (block_info *)((char *)b + BLOCK_INFO_SIZE + s);
  remainder->blockSize = b->blockSize - BLOCK_INFO_SIZE - s;
  remainder->next = b->next;
  remainder->prev = b;
  remainder->isFree = 1;
  

  b->blockSize = s;
  b->next = remainder;
  if (remainder->next)
    remainder->next->prev = remainder;
}

void *ff_malloc(size_t size) {
    if (size <= 0) {
        fprintf(stderr, "The requested size should be a positive integer\n");
        return NULL;
    }
    block_info * newblock;
    block_info * lastblock;
    if (begin == NULL) { // very first malloc call
        newblock = get_newblock(size, NULL);
        if (newblock == NULL) {
            return NULL;
        }
        begin = newblock;
    } else { // not the 1st malloc call
        lastblock = begin;
        newblock = ff_find_free_region(size, &lastblock);
	//
	if (newblock) {
	  //maybe split here
	  if ((newblock->blockSize - size) >= (BLOCK_INFO_SIZE+4))
	    split_block(newblock, size);
	  newblock->isFree = 0;
	} else {
	  // no fitting block, get a new block
	  newblock = get_newblock(size, lastblock);
	  if (!newblock)
	    return NULL;
	} 	  
    }
    return (newblock+1);
}

block_info *fusion(block_info * b) {
  if (b->next && b->next->isFree) {
    b->blockSize += (BLOCK_INFO_SIZE + b->next->blockSize);
    b->next = b->next->next;
    if (b->next)
      b->next->prev = b;
  }
  return b;
}

void ff_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    block_info *block_info_ptr = (block_info *)ptr - 1;
    //assert(block_info_ptr->block == ptr);
    if (block_info_ptr->isFree) {
        fprintf(stderr, "double free!\n");
        //exit(EXIT_FAILURE);
        return;
    }
    block_info_ptr->isFree = 1;
    if (block_info_ptr->prev && block_info_ptr->prev->isFree)
      block_info_ptr = fusion(block_info_ptr->prev);
    if (block_info_ptr->next)
      fusion(block_info_ptr);
    else {
      //reach the end
      if (block_info_ptr->prev)
	block_info_ptr->prev->next = NULL;
      else //no more block
	begin = NULL;
    }
}   

block_info *bf_find_free_region(size_t size, block_info **lastblock_ptr) {
    block_info * curr_block = begin;
    size_t difference = ULONG_MAX;
    block_info * bestfit = NULL;
    while (curr_block != NULL) {
        if (curr_block->isFree && curr_block->blockSize >= size) {
            if (curr_block->blockSize-size < difference) {
                difference = curr_block->blockSize-size;
                bestfit = curr_block;
            }
        }
        *lastblock_ptr = curr_block;
        curr_block = curr_block->next;
    }

    return bestfit;
}

void *bf_malloc(size_t size) {
    if (size <= 0) {
        fprintf(stderr, "The requested size should be a positive integer\n");
        return NULL;
    }
    block_info * newblock, *lastblock;
    if (begin == NULL) { // very first malloc call
        newblock = get_newblock(size, NULL);
        if (newblock == NULL) {
            return NULL;
        }
        begin = newblock;
    } else { // not the 1st malloc call
        lastblock = begin;
        newblock = bf_find_free_region(size, &lastblock);
        if (newblock) {
	  //maybe split here
	  if ((newblock->blockSize - size) >= (BLOCK_INFO_SIZE+4))
	    split_block(newblock, size);
	  newblock->isFree = 0;
	} else {
	  // no fitting block, get a new block
	  newblock = get_newblock(size, lastblock);
	  if (!newblock)
	    return NULL;
	} 	  
    }
    return (newblock+1);
}

void bf_free(void *ptr) {
    ff_free(ptr); //same implementation as before
}   


