#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

#include "myalloc.h"

// Global variables
void *_arena_start = NULL;
void *_arena_end = NULL;
size_t _arena_size = 0;

int statusno = 0;


int myinit(size_t size) {
    printf("Initializing arena:\n");
    printf("...requested size %zu bytes\n", size);

    // Validate the requested size
    if (size <= 0 || size > MAX_ARENA_SIZE) {
        statusno = ERR_BAD_ARGUMENTS;
        return statusno;
    }

    // Get the system's page size
    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize == -1) {
        perror("sysconf");
        statusno = ERR_SYSCALL_FAILED;
        return statusno;
    }
    printf("...pagesize is %ld bytes\n", pagesize);

    // Adjust the size to be a multiple of the page size
    size_t adjusted_size = ((size + pagesize - 1) / pagesize) * pagesize;
    printf("...adjusting size with page boundaries\n");
    printf("...adjusted size is %zu bytes\n", adjusted_size);

    // Map the memory arena using mmap()
    printf("...mapping arena with mmap()\n");
    void *arena = mmap(NULL, adjusted_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arena == MAP_FAILED) {
        perror("mmap");
        statusno = ERR_SYSCALL_FAILED;
        return statusno;
    }

    // Set global variables
    _arena_start = arena;
    _arena_size = adjusted_size;
    _arena_end = (char *)_arena_start + _arena_size;

    printf("...arena starts at %p\n", _arena_start);
    printf("...arena ends at %p\n", _arena_end);

    statusno = 0;
    return 0;
}


int mydestroy() {
    printf("Destroying Arena:\n");

    if (_arena_start == NULL) {
        statusno = ERR_UNINITIALIZED;
        return statusno;
    }

    printf("...unmapping arena with munmap()\n");
    if (munmap(_arena_start, _arena_size) == -1) {
        perror("munmap");
        statusno = ERR_SYSCALL_FAILED;
        return statusno;
    }

    // Reset global variables to their initial state
    _arena_start = NULL;
    _arena_end = NULL;
    _arena_size = 0;

    statusno = 0;
    return 0;
}
