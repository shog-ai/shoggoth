#include "../../camel.h"

#include "math.h"

#include <stdlib.h>

typedef struct {
  int int_value;
} my_fixture_t;

void test_add(test_t *test) {
  USE_SETUP(my_fixture_t);

  int int_value = SETUP_VALUE->int_value;

  int result = add(1, int_value);

  ASSERT_EQ(result, 1000, "The value should be 1000");
}

VALUE_T my_setup() {
  INIT_SETUP(my_fixture_t);

  SETUP_VALUE->int_value = 999;

  SETUP_RETURN();
}

void my_teardown(test_t *test) {
  USE_SETUP(my_fixture_t);

  FREE_SETUP();
}

void first_suite(suite_t *suite) {
  test_t *test_case = ADD_TEST(test_add);
  ADD_SETUP(test_case, my_setup);
  ADD_TEARDOWN(test_case, my_teardown);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(UNIT);

  ADD_SUITE(first_suite);

  CAMEL_END();
}
