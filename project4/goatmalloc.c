#include <stddef.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "goatmalloc.h"

void * arena_start = NULL;
int heapSize = 0;
int fd = -1;
node_t *freelist;
int statusno = ERR_UNINITIALIZED;

int init(size_t size){
    if (size > (SIZE_MAX - getpagesize())) {
        statusno = ERR_BAD_ARGUMENTS;
        return -2;
    }
    
    int pageSize = getpagesize();
    heapSize = ((size + pageSize - 1) / pageSize) * pageSize; //set heap size

    fd = open("/dev/zero", O_RDWR);
    if(fd == -1){
        return 1;
    }
    arena_start = mmap(NULL, heapSize, PROT_READ | PROT_WRITE , MAP_PRIVATE , fd , 0); //estab arena start
    if(arena_start == MAP_FAILED){
        statusno = ERR_UNINITIALIZED;
        return -5;
    }
    statusno = 0; //set status
    void *endAddress = (char *)arena_start + heapSize;
    printf("Initializing arena:\n...requested size %zu bytes\n...pagesizing is %d bytes\n...adjusting with page boundaries\n...adjusted size is %d bytes\n...mapping arena with mmap()\n...arena starts with %p\n...arena ends at %p\n...initializing header for initial free chunk\n...header size is 32 bytes", size, pageSize, heapSize, arena_start, endAddress);
    fflush(stdout);

    freelist = (node_t*)arena_start; //freelist setting
    freelist->size = heapSize - sizeof(node_t);
    freelist->is_free = 1;
    freelist->fwd = NULL;
    freelist->bwd = NULL;
    
    return heapSize;
}

int destroy(){
    if(statusno == ERR_UNINITIALIZED){
        return statusno;
    }
    printf("Destroying Arena:\n...unmapping arena with munmap()\n");
    fflush(stdout);
    if(munmap(arena_start, heapSize) == -1) {
        statusno = ERR_UNINITIALIZED;
        return ERR_UNINITIALIZED;
    }
    if(fd != -1){
        close(fd);
        fd = -1;
    }
    arena_start = NULL;
    heapSize = 0;

    return 0;
}

void* walloc(size_t size){
    if(arena_start == NULL){
        statusno = ERR_UNINITIALIZED;
        return NULL;
    }
    node_t *curr = freelist;
    while(curr){
        if(curr->is_free && curr->size >= size){
            if(curr->size >= size + sizeof(node_t) + 8){ 
                node_t *newBlock = (node_t*)((char*)curr + sizeof(node_t) + size);
                newBlock->size = curr->size - sizeof(node_t) - size;
                newBlock->is_free = 1;
                newBlock->fwd = curr->fwd;
                newBlock->bwd = curr;
                if(curr->fwd){
                    curr->fwd->bwd = newBlock;
                }
                curr->fwd = newBlock;
                curr->size = size;
            }
            curr->is_free = 0;
            return (void*)(curr + 1);
        }
        curr = curr->fwd;
    }
    statusno = ERR_OUT_OF_MEMORY;

    return NULL;
}

void wfree(void *ptr) {
    if (!ptr) {
        return;
    }
    node_t *block = (node_t*)ptr - 1;
    block->is_free = 1;
    if (block->fwd && block->fwd->is_free) { //if next block free, coalesce forwards
        block->size += sizeof(node_t) + block->fwd->size;
        block->fwd = block->fwd->fwd;
        if (block->fwd) {
            block->fwd->bwd = block;
        }
    }

    if (block->bwd && block->bwd->is_free) { //if prev block free, coalesce backwards
        block->bwd->size += sizeof(node_t) + block->size;
        block->bwd->fwd = block->fwd;
        if (block->fwd) {
            block->fwd->bwd = block->bwd;
        }
        block = block->bwd;
    }

    if (!block->bwd) { //if newly freed or coal. block is at the start of the arena, update freelist
        freelist = block;
    }

    block->bwd = NULL;
    block->fwd = NULL;
}

void* wrealloc(void* ptr, size_t size){ //INCOMPLETE
    if (!ptr) {
        return walloc(size);
    }

    if (size == 0) {
        wfree(ptr);
        return NULL;
    }

    node_t* block = (node_t*)ptr - 1; //get header
    size_t cursize = block->size;

    if (size <= cursize) { //return the same pointer if not above size
        return ptr;
    }

    if (block->fwd && block->fwd->is_free && (block->size + block->fwd->size + sizeof(node_t) >= size)) { //check if the next block is free and has enough space to expand
        node_t* nxtblock = block->fwd;
        block->size += sizeof(node_t) + nxtblock->size;
        block->fwd = nxtblock->fwd;
        if (block->fwd) {
            block->fwd->bwd = block;
        }
        return ptr;
    }
    void* newptr = walloc(size); //allocate a new block
    if (!newptr) {
        return NULL;
    }

    size_t copysize = (cursize < size) ? cursize : size;
    memcpy(newptr, ptr, copysize);

    wfree(ptr); //free the old block

    return newptr;
}
