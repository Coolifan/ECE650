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
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
block_info *begin_free = NULL;
__thread block_info *begin_TLS_free = NULL;


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

block_info *bf_find_free_region(size_t size, block_info *whichhead) {
    block_info * curr_block = whichhead;
    size_t difference = ULONG_MAX;
    block_info * bestfit = NULL;
    
    while (curr_block != NULL) {
        if (curr_block->blockSize == size) {
            return curr_block;
        }
        if (curr_block->blockSize > size) {
            if (curr_block->blockSize-size < difference) {
                difference = curr_block->blockSize-size;
                bestfit = curr_block;
            }
        }
        curr_block = curr_block->nextfree;
    }
    
    return bestfit;
}

void *split_block(block_info * b, size_t size, block_info ** whichhead) {
    block_info* remainder = (block_info *)((char *)b + BLOCK_INFO_SIZE + size); // new meta position
    remainder->nextfree = b->nextfree;
    remainder->prevfree = b->prevfree;
    remainder->blockSize = b->blockSize - BLOCK_INFO_SIZE - size;
    b->blockSize = size;
    //update block info inside the free LL
    if (b->prevfree == NULL) {
        // the head block in LL
        *whichhead = remainder;
    } else { //not the head block, then update prev block's next block
        b->prevfree->nextfree = remainder;
    }
    if (b->nextfree) {
        //not the tail block in LL
        b->nextfree->prevfree = remainder;
    }
    
    return b+1;
}

void remove_freeblock(block_info * b, block_info ** whichhead) {
    if (b->prevfree == NULL) { //first block in LL
        *whichhead = b->nextfree;
    } else { // not the head block in LL
        b->prevfree->nextfree = b->nextfree;
    }
    if (b->nextfree) { //not the tail block
        b->nextfree->prevfree = b->prevfree;
    }
    b->nextfree = NULL;
    b->prevfree = NULL;
}

void *ts_malloc_lock(size_t size) {
    if (size <= 0) {
        fprintf(stderr, "The requested size should be a positive integer\n");
        return NULL;
    }
    
    void * newaddr = NULL; //the return address malloc'd
    
    pthread_mutex_lock(&lock);
    if (begin_free == NULL) { // need to sbrk() a new block
        block_info * newblock = get_newblock(size);
        newaddr = newblock+1;
    } else { // there are some free blocks in free LL
        block_info * freeblock = bf_find_free_region(size, begin_free);
        if (freeblock) { // free block found in free LL
            //try to split it
            if (freeblock->blockSize - size >= BLOCK_INFO_SIZE) {
                newaddr = split_block(freeblock, size, &begin_free);
            } else { // not big enough to split, just remove the block from LL
                remove_freeblock(freeblock, &begin_free);
                newaddr = freeblock+1;
            }
        } else { // no proper free space found, get a new block
            block_info *newblock = get_newblock(size);
            newaddr = newblock+1;
        }
    }
    pthread_mutex_unlock(&lock);
    return newaddr;
}

void add_freeblock(block_info *b, block_info **whichhead) {
    block_info **curr_block = whichhead;
    block_info *last = NULL; //useful for inserting the tail block
    while (*curr_block) {
        if (b < *curr_block) {
            b->prevfree = (*curr_block)->prevfree;
            b->nextfree = *curr_block;
            (*curr_block)->prevfree = b;
            *curr_block = b;
            return;
        }
        last = *curr_block;
        curr_block = &((*curr_block)->nextfree);
    }
    // insert tail block
    b->prevfree = last;
    b->nextfree = *curr_block; //null
    *curr_block = b;
}

void merge_block(block_info * b_this, block_info * b_next) {
    if (b_this == NULL)
        return;
    
    if (b_next) {
        block_info * real_neighbor = (block_info *)((char *)b_this + BLOCK_INFO_SIZE + b_this->blockSize);
        if (real_neighbor == b_next) {
            // physically adjacent blocks, need merging
            if (b_next->nextfree) {
                b_next->nextfree->prevfree = b_this;
            }
            b_this->nextfree = b_next->nextfree;
            b_this->blockSize += (BLOCK_INFO_SIZE+b_next->blockSize);
        }
    }
}

void ts_free_lock(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    pthread_mutex_lock(&lock);
    // find the address of the meta struct
    block_info *block_tobefreed = (block_info *)ptr - 1;
    add_freeblock(block_tobefreed, &begin_free);
    // try to merge adjacent blocks
    block_info * nextblock = block_tobefreed->nextfree;
    block_info * prevblock = block_tobefreed->prevfree;
    
    if (!prevblock) {
        merge_block(block_tobefreed, nextblock);
    } else {
        merge_block(prevblock, block_tobefreed);
        if (prevblock->nextfree == nextblock) {
            // 3 adjacent blocks need to be merged
            merge_block(prevblock, nextblock);
        } else { // prevblock and thisblock are not actually adjacent in mem
            merge_block(block_tobefreed, nextblock);
        }
    }
    pthread_mutex_unlock(&lock);
}

//******************** no lock ***********************//
void *ts_malloc_nolock(size_t size) {
    if (size <= 0) {
        fprintf(stderr, "The requested size should be a positive integer\n");
        return NULL;
    }
    
    void * newaddr = NULL; //the return address malloc'd
    
    if (begin_TLS_free == NULL) { // need to sbrk() a new block
        pthread_mutex_lock(&lock);
        block_info * newblock = get_newblock(size);
        pthread_mutex_unlock(&lock);
        newaddr = newblock+1;
    } else { // there are some free blocks in free LL
        block_info * freeblock = bf_find_free_region(size, begin_TLS_free);
        if (freeblock) { // free block found in free LL
            //try to split it
            if (freeblock->blockSize - size >= BLOCK_INFO_SIZE) {
                newaddr = split_block(freeblock, size, &begin_TLS_free);
            } else { // not big enough to split, just remove the block from LL
                remove_freeblock(freeblock, &begin_TLS_free);
                newaddr = freeblock+1;
            }
        } else { // no proper free space found, get a new block
            pthread_mutex_lock(&lock);
            block_info *newblock = get_newblock(size);
            pthread_mutex_unlock(&lock);
            newaddr = newblock+1;
        }
    }
    
    return newaddr;
}

void ts_free_nolock(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    
    // find the address of the meta struct
    block_info *block_tobefreed = (block_info *)ptr - 1;
    add_freeblock(block_tobefreed, &begin_TLS_free);
    // try to merge adjacent blocks
    block_info * nextblock = block_tobefreed->nextfree;
    block_info * prevblock = block_tobefreed->prevfree;
    
    if (!prevblock) {
        merge_block(block_tobefreed, nextblock);
    } else {
        merge_block(prevblock, block_tobefreed);
        if (prevblock->nextfree == nextblock) {
            // 3 adjacent blocks need to be merged
            merge_block(prevblock, nextblock);
        } else { // prevblock and thisblock are not actually adjacent in mem
            merge_block(block_tobefreed, nextblock);
        }
    }
    
}
