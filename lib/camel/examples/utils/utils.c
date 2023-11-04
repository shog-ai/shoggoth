#include "../../camel.h"

#include "math.h"

// This example demonstrates testing with various utilities provided by camel

void my_test(test_t *test) {
  char *content = READ_FILE("./hello.txt");

  ASSERT_EQ_STR(content, "Hello world\n", "strings should be equal");

  bool equal = FILES_EQUAL("./hello.txt", "hello_copy.txt");

  ASSERT(equal, "files should be equal");
}

void first_suite(suite_t *suite) {
  //

  ADD_TEST(my_test);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(UNIT);

  ADD_SUITE(first_suite);

  CAMEL_END();
}
