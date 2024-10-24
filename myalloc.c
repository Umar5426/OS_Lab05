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

    // Initialize the first free chunk
    node_t *initial_chunk = (node_t *)arena_start;
    initial_chunk->size = arena_size - sizeof(node_t);
    initial_chunk->is_free = 1;
    initial_chunk->fwd = NULL;
    initial_chunk->bwd = NULL;

    printf("...initialized first free chunk at %p with size %zu bytes\n", initial_chunk, initial_chunk->size);

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

void* myalloc(size_t size) {
    printf("Allocating memory of size %zu bytes\n", size);

    // Check if the arena is initialized
    if (arena_start == NULL) {
        statusno = ERR_UNINITIALIZED;
        printf("...arena is uninitialized\n");
        return NULL;
    }

    if (size == 0) {
        statusno = ERR_BAD_ARGUMENTS;
        printf("...requested size is zero\n");
        return NULL;
    }

    // Start from the beginning of the arena
    node_t *current = (node_t *)arena_start;

    // Traverse the linked list to find a suitable free chunk
    while (current != NULL) {
        printf("...checking chunk at %p: size=%zu, is_free=%d\n", current, current->size, current->is_free);

        if (current->is_free && current->size >= size) {
            printf("...found suitable chunk\n");

            // Determine if splitting is necessary
            size_t total_size_needed = size + sizeof(node_t);
            if (current->size >= total_size_needed) {
                printf("...splitting chunk\n");

                // Create new chunk
                node_t *new_chunk = (node_t *)((char *)current + sizeof(node_t) + size);
                new_chunk->size = current->size - size - sizeof(node_t);
                new_chunk->is_free = 1;
                new_chunk->fwd = current->fwd;
                new_chunk->bwd = current;

                if (current->fwd != NULL) {
                    current->fwd->bwd = new_chunk;
                }

                current->size = size;
                current->is_free = 0;
                current->fwd = new_chunk;

                printf("...chunk split into allocated chunk of size %zu and free chunk of size %zu\n", current->size, new_chunk->size);
            } else {
                printf("...allocating entire chunk without splitting\n");
                current->is_free = 0;
            }

            void *allocated_memory = (void *)((char *)current + sizeof(node_t));
            printf("...allocation successful, memory starts at %p\n", allocated_memory);
            statusno = 0;
            return allocated_memory;
        }

        current = current->fwd;
    }

    printf("...no suitable free chunk found\n");
    statusno = ERR_OUT_OF_MEMORY;
    return NULL;
}

void myfree(void *ptr) {
    printf("Freeing allocated memory:\n");

    // Check if the arena is initialized
    if (arena_start == NULL) {
        statusno = ERR_UNINITIALIZED;
        return;
    }

    if (ptr == NULL) {
        statusno = ERR_BAD_ARGUMENTS;
        return;
    }

    // Calculate the address of the chunk header
    node_t *current = (node_t *)((char *)ptr - sizeof(node_t));
    printf("...accessing chunk header at %p\n", current);

    // Mark the chunk as free
    current->is_free = 1;
    printf("...chunk of size %zu marked as free\n", current->size);

    // Coalesce with previous chunk if it's free
    if (current->bwd != NULL && current->bwd->is_free) {
        printf("...coalescing with previous free chunk at %p\n", current->bwd);
        current->bwd->size += sizeof(node_t) + current->size;
        current->bwd->fwd = current->fwd;

        if (current->fwd != NULL) {
            current->fwd->bwd = current->bwd;
        }

        current = current->bwd;
    }

    // Coalesce with next chunk if it's free
    if (current->fwd != NULL && current->fwd->is_free) {
        printf("...coalescing with next free chunk at %p\n", current->fwd);
        current->size += sizeof(node_t) + current->fwd->size;
        current->fwd = current->fwd->fwd;

        if (current->fwd != NULL) {
            current->fwd->bwd = current;
        }
    }

    statusno = 0;
}