//
//  my_malloc.c
//  my_malloc
//
//  Created by Yifan Li on 2/4/18.
//  Copyright © 2018 Yifan Li. All rights reserved.
//
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include "my_malloc.h"
#include <pthread.h>

//void *begin = NULL;
block_info *begin_free = NULL;
__thread block_info *begin_TLS_free = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


block_info *get_newblock(size_t size) {
  block_info *newblock = sbrk(0);
  if (sbrk(size+BLOCK_INFO_SIZE) == (void *) -1) {
    fprintf(stderr, "sbrk failed\n");
    return NULL;
  }
  newblock->nextfree = NULL;
  newblock->prevfree = NULL;
  newblock->blockSize = size;

  return newblock;
}


void * split_block(block_info * b, size_t size, block_info ** head_free) {
  block_info* remainder = (block_info *)((char *)b + BLOCK_INFO_SIZE + size); // new meta position
  remainder->nextfree = b->nextfree;
  remainder->prevfree = b->prevfree;
  remainder->blockSize = b->blockSize - BLOCK_INFO_SIZE - size;
  b->blockSize = size;

  // update block info inside the free LL
  if (b->prevfree == NULL) {
    //the first block in free LL
    *head_free = remainder;
  } else {
    //not the first block in free LL
    b->prevfree->nextfree = remainder;
  }
  if (b->nextfree) {
    //not the last block in free LL
    b->nextfree->prevfree = remainder;
  }
  return (b+1);
}


block_info *bf_find_free_region(size_t size, block_info *head_free) {
  block_info * curr_block = head_free;
  size_t difference = ULONG_MAX;
  block_info * bestfit = NULL;

  while (curr_block != NULL) {
    if (curr_block->blockSize == size) {
      return curr_block;
    }
    if (curr_block->blockSize > size) {
      if (curr_block->blockSize - size < difference) {
	difference = curr_block->blockSize - size;
	bestfit = curr_block;
      }
    }
    curr_block = curr_block->nextfree;
  }

  return bestfit;
}

void *ts_malloc_lock(size_t size) {
  if (size <= 0) {
    fprintf(stderr, "The requested size should be a positive integer\n");
    return NULL;
  }

  void * newaddr = NULL; //the addr that will be returned

  pthread_mutex_lock(&lock);
  block_info * freeblock = bf_find_free_region(size, begin_free);
  if (freeblock) { //free block found, try to split here
    if (freeblock->blockSize - size >= BLOCK_INFO_SIZE) {
      newaddr = split_block(freeblock, size, &begin_free);
    } else { // not big enough to split into 2 parts.
      // just remove this block from free LL
      if (freeblock->prevfree == NULL) {//first block in LL
	begin_free = freeblock->nextfree;
      } else { // not the first block in LL
	freeblock->prevfree->nextfree = freeblock->nextfree;
      }
      if (freeblock->nextfree != NULL) { // not the last in LL
	freeblock->nextfree->prevfree = freeblock->prevfree;
      }
      freeblock->nextfree = NULL;
      freeblock->prevfree = NULL;
      newaddr = freeblock+1;
    }
  } else { // no proper free space found, get a new block
    block_info * newblock = get_newblock(size);
    newaddr = newblock+1;
  }
  pthread_mutex_unlock(&lock);
  return newaddr;
}


// add the newly freed block into the free LL
void addToLL(block_info * node, block_info ** head_free) {
  block_info **curr_block = head_free;
  block_info *last = NULL;
  while (*curr_block) {
    if (*curr_block > node) {
      node->nextfree = *curr_block;
      node->prevfree = (*curr_block)->prevfree;
      (*curr_block)->prevfree = node;
      *curr_block = node;
      return;
    }
    last = *curr_block;
    curr_block = &((*curr_block)->nextfree);
  }
  node->prevfree = last;
  node->nextfree = *curr_block;
  *curr_block = node;
}


// modified merge function to fit free-block-only LL
void merge_block(block_info * b_curr, block_info * b_next) {
  if (!b_curr || !b_next)
    return;
  block_info * neighbor = (block_info *)((char *)b_curr + BLOCK_INFO_SIZE + b_curr->blockSize);
  if (neighbor == b_next) {
    //two blocks are neighbors, need to merge
    if (b_curr->nextfree->nextfree) {
      b_curr->nextfree->nextfree->prevfree = b_curr;
    }
    b_curr->nextfree = b_curr->nextfree->nextfree;
    b_curr->blockSize += (BLOCK_INFO_SIZE + b_next->blockSize);
  }
}


void ts_free_lock(void *ptr) {
  if (ptr == NULL) {
    return;
  } else {
    pthread_mutex_lock(&lock);
    // find the address of the meta struct
    block_info *block_info_ptr = (block_info *)ptr - 1;
    addToLL(block_info_ptr, &begin_free);

    // try to merge
    block_info * nextblock = block_info_ptr->nextfree;
    block_info * prevblock = block_info_ptr->prevfree;

    if (!prevblock) {
      merge_block(block_info_ptr, nextblock);
    } else {
      merge_block(prevblock, block_info_ptr);
      if (prevblock->nextfree == nextblock) {
	// 3 adjacent blocks need to be merged
	merge_block(prevblock, nextblock);
      } else {
	merge_block(block_info_ptr, nextblock);
      }
    }
    pthread_mutex_unlock(&lock);
  }
}


//******************** no lock ***********************//
void *ts_malloc_nolock(size_t size) {
  if (size <= 0) {
    fprintf(stderr, "The requested size should be a positive integer\n");
    return NULL;
  }

  void * newaddr = NULL; //the addr that will be returned
  /* non-locking version */
  block_info * freeblock = bf_find_free_region(size, begin_TLS_free);
  if (freeblock) { //free block found, try to split here
    if (freeblock->blockSize - size >= BLOCK_INFO_SIZE) {
      newaddr = split_block(freeblock, size, &begin_TLS_free);
    } else { // not big enough to split into 2 parts.
      // just remove this block from free LL
      if (freeblock->prevfree == NULL) {//first block in LL
	begin_TLS_free = freeblock->nextfree;
      } else { // not the first block in LL
	freeblock->prevfree->nextfree = freeblock->nextfree;
      }
      if (freeblock->nextfree != NULL) { // not the last in LL
	freeblock->nextfree->prevfree = freeblock->prevfree;
      }
      freeblock->nextfree = NULL;
      freeblock->prevfree = NULL;
      newaddr = freeblock+1;
    }
    
  } else { // no proper free space found, get a new block
    pthread_mutex_lock(&lock);
    //lock exception : acquire before sbrk and release immediately
    block_info * newblock = sbrk(size+BLOCK_INFO_SIZE);
    pthread_mutex_unlock(&lock);

    newblock->blockSize = size;
    newblock->nextfree = NULL;
    newblock->prevfree = NULL;

    newaddr = newblock+1;
  }

  return newaddr;
}


void ts_free_nolock(void *ptr) {
  if (ptr == NULL) {
    return;
  } else {
    //non-locking version
    // find the address of the meta struct
    block_info *block_info_ptr = (block_info *)ptr - 1;
    addToLL(block_info_ptr, &begin_TLS_free);

    // try to merge
    block_info * nextblock = block_info_ptr->nextfree;
    block_info * prevblock = block_info_ptr->prevfree;

    if (!prevblock) {
      merge_block(block_info_ptr, nextblock);
    } else {
      merge_block(prevblock, block_info_ptr);
      if (prevblock->nextfree == nextblock) {
	// 3 adjacent blocks need to be merged
	merge_block(prevblock, nextblock);
      } else {
	merge_block(block_info_ptr, nextblock);
      }
    }
  }
}
