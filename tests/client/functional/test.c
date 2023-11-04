#include "../../../src/include/camel.h"

#include <stdlib.h>

void placeholder_test(test_t *test) {}

void test_suite(suite_t *suite) { ADD_TEST(placeholder_test); }

int main(int argc, char **argv) {
  CAMEL_BEGIN(FUNCTIONAL);

  ADD_SUITE(test_suite);

  CAMEL_END();
}
