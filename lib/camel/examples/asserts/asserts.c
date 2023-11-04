#include "../../camel.h"

#include "math.h"

// This example demonstrates using asserts in a test

void add_test(test_t *test) {
  ASSERT(true, "should be true");

  ASSERT_FALSE(false, "should be false");

  ASSERT_EQ(88, 88, "should be equal");

  ASSERT_NEQ(88, 11, "should not be equal");

  char *null_ptr = NULL;
  ASSERT_NULL(null_ptr, "should be null");

  char *ptr = "hello";
  ASSERT_NOT_NULL(ptr, "should not be null");

  char *string1 = "hey";
  char *string2 = "hey";

  ASSERT_EQ_STR(string1, string2, "strings should be equal");

  ASSERT_EQ_BOOL(true, true, "bools should be equal");
}

void first_suite(suite_t *suite) {
  //

  ADD_TEST(add_test);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(UNIT);

  ADD_SUITE(first_suite);

  CAMEL_END();
}
