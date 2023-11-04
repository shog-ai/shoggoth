===============================================================================
CAMEL - SIMPLE TESTING FRAMEWORK
===============================================================================

Camel is a testing framework written in the C programming language, developed by Shoggoth Systems for use in the Shoggoth project (https://shoggoth.network).

It can be used for writing unit tests, integration tests, functional tests, and fuzz tests. Camel is the framework used for testing Shoggoth, Sonic, and other Shoggoth projects.

**WARNING**
This library is still early in development and should be considered unstable and experimental.



===============================================================================
HOW TO USE CAMEL
===============================================================================

Testing with Camel follows the below structure:


                                    TEST RUNNER
                                         |
                                         |
            --------------------------------------------------------
            |                            |                         |
        TEST SUITE                  TEST SUITE                 TEST SUITE
            |                            |                         |
            |                            |                         |
  ---------------------       ---------------------     ---------------------
  |      |     |      |       |      |     |      |     |      |     |      |
 TEST   TEST  TEST   TEST    TEST   TEST  TEST   TEST  TEST   TEST  TEST   TEST


Every test is a part of a test suite which contains one or more related tests. One or more test suites are then attached to a test runner which will run every test suite, and consequently every test case, sequentially or simultaneously.

To write tests for a group of functions, for example, a math library containing simple math functions like add, multiply, etc, you will need four files:

1. camel.h - the camel library header file
2. math.c - the C source file containing the functions you want to test
3. math.h - a header file for the above C file, containing the function signatures
4. test.c - a new file where you will write your test suites and test cases. 

Below is a demonstration of how to use the above structure:


The file containing functions we want to test ->
======================math.c====================
int add(int left, int right) {
  return left + right;
}

int subtract(int left, int right) {
  return left - right;
}

int multiply(int left, int right) {
  return left * right;
}

int divide(int left, int right) {
  return left / right;
}
================================================

The header for the above file ->
======================math.h====================
#ifndef MATH_EXAMPLE
#define MATH_EXAMPLE

int add(int left, int right);

int subtract(int left, int right);

int multiply(int left, int right);

int divide(int left, int right);

#endif
================================================

file containing tests ->
======================test.c====================
#include "camel.h"
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
================================================

compile math.c and test.c together and link the libcamel.a static library:

$ gcc math.c test.c libcamel.a -o test

Then run the test executable:

$ ./test

The executable should create output similar to the below:

STARTING UNIT TEST RUNNER ======================== ./examples/simple/simple.c

SUITE -> first_suite
PASS: add_test -> 266ns
PASS: subtract_test -> 111ns
PASS: multiply_test -> 99ns
PASS: divide_test -> 137ns
SUITE SUMMARY (first_suite): 4 passed, 0 failed, 5 asserts, 4 tests
TEST SUMMARY (./examples/simple/simple.c): 4 passed, 0 failed, 1 suites, 5 asserts, 4 tests
FINISHED TEST RUNNER ===================================




The above example is available in the examples directory at ./examples/simple/

You can run it with:

$ E=simple make run-example

More examples can  also be found in ./examples/



===============================================================================
COMMAND LINE FLAGS
===============================================================================

When running the test executable, you can provide some command line flags to customize its behavior such as whether to run the tests in parallel or not.

Here are the available flags:

=====================================================================
flag    || meaning        || description
=====================================================================
-f      || fail fast      || exits immediately a test fails 
=====================================================================
-p      || parralel       || runs all tests in parralel 
=====================================================================
-e      || exit code      || disables exiting with error codes if
        ||                || tests fail. When used, the test executable
        ||                || will always exit with 0.
=====================================================================



===============================================================================
BUILDING CAMEL
===============================================================================

Supported platforms
================================================

Camel currently supports only Linux and macOS operating systems.


Requirements
================================================

* GNU Make


Building
================================================

Clone the repository
============================

$ git clone https://github.com/shoggoth-systems/camel

cd into the cloned directory
============================

$ cd camel


Build with make
============================

$ make build

The above command will build a static library into ./target/libcamel.a which you can link with your project.



===============================================================================
DOCUMENTATION
===============================================================================

Detailed documentation on how to use Camel can be found at https://shoggoth.network/explorer/docs/camel



===============================================================================
CONTRIBUTING
===============================================================================

Please follow the Shoggoth contribution guidelines at https://shoggoth.network/explorer/docs#contributing
