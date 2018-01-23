//
//  my_malloc.h
//  my_malloc
//
//  Created by Yifan Li on 1/21/18.
//  Copyright © 2018 Yifan Li. All rights reserved.
//

#ifndef my_malloc_h
#define my_malloc_h

#include <stdio.h>

typedef struct _block_info {
    size_t blockSize;
    struct _block_info *next;
    struct _block_info *prev;
    int isFree;
    void *blockPtr;
} block_info;

#define BLOCK_INFO_SIZE sizeof(struct _block_info)

void *ff_malloc(size_t size);
void ff_free(void *ptr);

void *bf_malloc(size_t size);
void bf_free(void *ptr);


#endif /* my_malloc_h */