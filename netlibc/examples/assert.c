#include "../include/netlibc/assert.h"

int main() {
  ASSERT_TRUE(true, "assert should pass");

  ASSERT_FALSE(false, "assert should pass");

  ASSERT_TRUE(false, NULL);

  // ASSERT_TRUE(1 > 0, "assert should fail");

  return 0;
}
