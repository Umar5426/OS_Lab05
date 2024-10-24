#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

#include "myalloc.h"

// Global variables
void *arena_start = NULL;
void *arena_end = NULL;
size_t arena_size = 0;

int statusno = 0;

int myinit(size_t size) {
    printf("Initializing arena:\n");
    printf("...requested size %zu bytes\n", size);

    // Validate the requested size
    if (size == 0 || size > MAX_ARENA_SIZE) {
        statusno = ERR_BAD_ARGUMENTS;
        return ERR_BAD_ARGUMENTS;
    }

    // Get the system's page size
    size_t pagesize = (size_t)sysconf(_SC_PAGESIZE);
    if (pagesize == (size_t)-1) {
        perror("sysconf");
        statusno = ERR_SYSCALL_FAILED;
        return ERR_SYSCALL_FAILED;
    }
    printf("...pagesize is %zu bytes\n", pagesize);

    // Adjust the size
    size_t adjusted_size;
    if (size <= pagesize) {
        adjusted_size = pagesize;
    } else {
        adjusted_size = ((size + pagesize - 1) / pagesize) * pagesize;
    }
    printf("...adjusting size with page boundaries\n");
    printf("...adjusted size is %zu bytes\n", adjusted_size);

    // Map the memory arena using mmap()
    printf("...mapping arena with mmap()\n");
    void *arena = mmap(NULL, adjusted_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arena == MAP_FAILED) {
        perror("mmap");
        statusno = ERR_SYSCALL_FAILED;
        return ERR_SYSCALL_FAILED;
    }

    // Set global variables
    arena_start = arena;
    arena_size = adjusted_size;
    arena_end = (char *)arena_start + arena_size;

    printf("...arena starts at %p\n", arena_start);
    printf("...arena ends at %p\n", arena_end);
    printf("...arena_size is %zu bytes\n", arena_size);

    statusno = 0;
    return (int)arena_size; // Return the adjusted arena size
}


int mydestroy() {
    printf("Destroying Arena:\n");

    if (arena_start == NULL) {
        statusno = ERR_UNINITIALIZED;
        return ERR_UNINITIALIZED;
    }

    printf("...unmapping arena with munmap()\n");
    if (munmap(arena_start, arena_size) == -1) {
        perror("munmap");
        statusno = ERR_SYSCALL_FAILED;
        return ERR_SYSCALL_FAILED;
    }

    // Reset global variables to their initial state
    arena_start = NULL;
    arena_end = NULL;
    arena_size = 0;

    statusno = 0;
    return 0;
}

//Part 1 -- Succeeded