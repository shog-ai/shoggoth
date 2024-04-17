#ifndef NETLIBC_MEM_H
#define NETLIBC_MEM_H

#include "../netlibc.h"

void *__netlibc_malloc(size_t size, char *file, u64 line);

void *__netlibc_realloc(void *pre_allocated, size_t new_size, char *file,
                        u64 line);

void *__netlibc_calloc(u64 count, size_t size, char *file, u64 line);

void __netlibc_free(void *allocated, char *file, u64 line);

#define nmalloc(size) __netlibc_malloc(size, __FILE__, __LINE__);

#define ncalloc(count, size) __netlibc_calloc(count, size, __FILE__, __LINE__);

#define nrealloc(pre_allocated, new_size)                                      \
  __netlibc_realloc(pre_allocated, new_size, __FILE__, __LINE__);

#define nfree(allocated) __netlibc_free(allocated, __FILE__, __LINE__);

#endif
