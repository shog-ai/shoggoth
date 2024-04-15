#define MEM_DEBUG 1
#include "../include/netlibc/mem.h"
#include "../include/netlibc.h"

int main() {
  NETLIBC_INIT();

  void *allocated = ncalloc(2, 100);

  void *allocated2 = nmalloc(100);

  void *allocated3 = nrealloc(allocated, 100);

  nfree(allocated2);
  nfree(allocated3);

  return 0;
}
