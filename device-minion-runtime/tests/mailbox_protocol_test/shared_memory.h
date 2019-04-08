#include <stdint.h>
#include <stdlib.h>

int create_shared_memory(const char * filename, size_t size, void ** pshared_mem);
int delete_shared_memory(const char * filename, void * shared_mem, size_t size);
int open_shared_memory(const char * filename, size_t size, void ** pshared_mem);
int close_shared_memory(void * shared_mem, size_t size);
