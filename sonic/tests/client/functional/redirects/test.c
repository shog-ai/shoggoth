#include "../../../../include/camel.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void many_redirects(test_t *test) {
  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./many_redirects", NULL);
  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  char *expected_content = "STATUS:3|BODY:REDIRECT5";
  char *content = READ_FILE("./client-output.txt");
  ASSERT_EQ_STR(expected_content, content, NULL);
  free(content);
}

void hops_limit(test_t *test) {
  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./hops_limit", NULL);
  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  char *expected_content = "MAX HOPS";
  char *content = READ_FILE("./client-output.txt");
  ASSERT_EQ_STR(expected_content, content, NULL);
  free(content);
}

void no_redirect(test_t *test) {
  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./no_redirect", NULL);
  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  char *expected_content = "STATUS:12|BODY:REDIRECT1";
  char *content = READ_FILE("./client-output.txt");
  ASSERT_EQ_STR(expected_content, content, NULL);
  free(content);
}

VALUE_T setup() {
  INIT_SETUP(process_t);
  free(SETUP_VALUE);

  process_t *server_process =
      RUN_EXECUTABLE("./server-output.txt", "./local_server", NULL);

  SETUP_VALUE = server_process;

  usleep(100000);

  SETUP_RETURN();
}

void teardown(test_t *test) {
  if (test->test_failed) {
    printf("Dumping server output:\n");
    char *server_output = READ_FILE("./server-output.txt");
    printf("%s\n", server_output);
    free(server_output);

    printf("Dumping client output:\n");
    char *client_output = READ_FILE("./client-output.txt");
    printf("%s\n", client_output);
    free(client_output);
  }

  USE_SETUP(process_t);

  KILL_PROCESS(SETUP_VALUE, SIGTERM);
  int server_status = AWAIT_PROCESS(SETUP_VALUE);
  ASSERT_EQ(0, server_status, NULL);

  FREE_SETUP();
}

void test_suite(suite_t *suite) {
  test_t *many_redirects_test = ADD_REPEAT_TEST(many_redirects, 1);
  ADD_SETUP(many_redirects_test, setup);
  ADD_TEARDOWN(many_redirects_test, teardown);

  test_t *hops_limit_test = ADD_REPEAT_TEST(hops_limit, 1);
  ADD_SETUP(hops_limit_test, setup);
  ADD_TEARDOWN(hops_limit_test, teardown);

  test_t *no_redirect_test = ADD_REPEAT_TEST(no_redirect, 5);
  ADD_SETUP(no_redirect_test, setup);
  ADD_TEARDOWN(no_redirect_test, teardown);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(FUNCTIONAL);

  ADD_SUITE(test_suite);

  CAMEL_END();
}
