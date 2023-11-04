#define _GNU_SOURCE

#include "../../../src/include/camel.h"

#include "../../../src/client/client.h"
#include "../../../src/profile/profile.h"

#include <stdlib.h>

void client_test_suite(suite_t *suite) {}

int main(int argc, char **argv) {
  CAMEL_BEGIN(UNIT);

  ADD_SUITE(client_test_suite);

  CAMEL_END();
}
