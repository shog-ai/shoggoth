#include "../../camel.h"

#include "math.h"

// This example demonstrates testing simple math functions

void add_test(test_t *test) {
  int result = add(1, 1);

  ASSERT_EQ(result, 2, "The value should be 2");

  result = add(99, 1);

  ASSERT_EQ(result, 100, "The value should be 100");
}

void subtract_test(test_t *test) {
  int result = subtract(155, 3);

  ASSERT_EQ(result, 152, "The value should be 152");
}

void multiply_test(test_t *test) {
  int result = multiply(5, 5);

  ASSERT_EQ(result, 25, "The value should be 152");
}

void divide_test(test_t *test) {
  int result = divide(8, 2);

  ASSERT_EQ(result, 4, "The value should be 152");
}

void first_suite(suite_t *suite) {
  ADD_TEST(add_test);
  ADD_TEST(subtract_test);
  ADD_TEST(multiply_test);
  ADD_TEST(divide_test);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(UNIT);

  ADD_SUITE(first_suite);

  CAMEL_END();
}
