//
//  my_malloc.h
//  my_malloc
//
//  Created by Yifan Li on 2/4/18.
//  Copyright © 2018 Yifan Li. All rights reserved.
//

#ifndef my_malloc_h
#define my_malloc_h

#include <stdio.h>
#include <stdlib.h>

typedef struct _block_info {
  size_t blockSize;
  struct _block_info *nextfree;
  struct _block_info *prevfree;
  //int isFree;
} block_info;

#define BLOCK_INFO_SIZE sizeof(block_info)

void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);

void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);


#endif /* my_malloc_h */
