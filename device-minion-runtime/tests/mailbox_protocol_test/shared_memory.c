#include <errno.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <sys/shm.h> 
#include <sys/stat.h> 
#include <sys/mman.h>
#include <sys/types.h> 

#include "shared_memory.h"

int create_shared_memory(const char * filename, size_t size, void ** pshared_mem) {
    void * ptr;
    int shared_fd;

    shared_fd = shm_open(filename, O_CREAT | O_RDWR, 0666);
    if (0 == shared_fd) {
        printf("shm_open() failed with error 0x%x!\n", errno);
        return -1;
    }

    if (0 != ftruncate(shared_fd, (__off_t)size)) {
        printf("ftruncate() failed with error 0x%x!\n", errno);
        shm_unlink(filename);
        return -1;
    }

    printf("Created shared memory object '%s'\n", filename);

    ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
    if (MAP_FAILED == ptr) {
        printf("mmap() failed with error 0x%x!\n", errno);
        shm_unlink(filename);
        return -1;
    }

    printf("Mapped shared memory at address %p\n", ptr);
    *pshared_mem = ptr;
    return 0;
}

int delete_shared_memory(const char * filename, void * shared_mem, size_t size) {
    if (0 != munmap(shared_mem, size)) {
        printf("munmap() failed with error 0x%x!\n", errno);
        return -1;
    }
    printf("Unmapped shared memory at address %pP\n", shared_mem);
    if (0 != shm_unlink(filename)) {
        printf("shm_unlink() failed with error 0x%x!\n", errno);
        return -1;
    }
    printf("Deleted shared memory object '%s'\n", filename);
    return 0;
}

int open_shared_memory(const char * filename, size_t size, void ** pshared_mem) {
    void * ptr;
    int shared_fd;

    shared_fd = shm_open(filename, O_RDWR, 0666);
    if (0 == shared_fd) {
        printf("shm_open() failed with error 0x%x!\n", errno);
        return -1;
    }

    ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
    if (MAP_FAILED == ptr) {
        printf("mmap() failed with error 0x%x!\n", errno);
        shm_unlink(filename);
        return -1;
    }

    printf("Mapped shared memory at address %p\n", ptr);
    *pshared_mem = ptr;
    return 0;
}

int close_shared_memory(void * shared_mem, size_t size) {
    if (0 != munmap(shared_mem, size)) {
        printf("munmap() failed with error 0x%x!\n", errno);
        return -1;
    }
    printf("Unmapped shared memory at address %p\n", shared_mem);
    return 0;
}
