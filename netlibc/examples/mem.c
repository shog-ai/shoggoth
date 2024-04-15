// #define MEMDEBUG
#include "../include/netlibc/mem.h"
#include "../include/netlibc.h"

int main() {
  NETLIBC_INIT(MEM_DEBUG_ENABLED);

  void *allocated = nmalloc(100);

  void *allocated2 = nmalloc(100);

  void *allocated3 = nmalloc(100);

  nfree(allocated);
  nfree(allocated2);
  // nfree(allocated3);

  return 0;
}
