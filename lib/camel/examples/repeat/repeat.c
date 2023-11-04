#include "../../camel.h"

#include "math.h"

// This example demonstrates testing simple math functions

void my_test(test_t *test) {
  int result = divide(8, 2);

  ASSERT_EQ(result, 4, "The value should be 152");
}

void first_suite(suite_t *suite) {
  // runs the test 20 times
  ADD_REPEAT_TEST(my_test, 20);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(UNIT);

  ADD_SUITE(first_suite);

  CAMEL_END();
}
