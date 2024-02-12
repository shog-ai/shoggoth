#include "../../../../include/camel.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void get_200(test_t *test) {
  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./get_200", NULL);
  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  char *expected_content = "STATUS:3|BODY:Hello world";
  char *content = READ_FILE("./client-output.txt");
  ASSERT_EQ_STR(expected_content, content, NULL);
  free(content);
}

void get_404(test_t *test) {
  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./get_404", NULL);
  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  char *expected_content = "STATUS:21|BODY:404 NOT FOUND";
  char *content = READ_FILE("./client-output.txt");
  ASSERT_EQ_STR(expected_content, content, NULL);
  free(content);
}

void post_with_body(test_t *test) {
  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./post_with_body", NULL);
  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  char *client_content = READ_FILE("./client-output.txt");
  char *expected_client_content = "STATUS:3|BODY:DONE";
  ASSERT_EQ_STR(expected_client_content, client_content, NULL);
  free(client_content);

  char *server_content = READ_FILE("./server-output.txt");
  char *expected_server_content = "REQUEST BODY:THEBODY1234";
  ASSERT_EQ_STR(expected_server_content, server_content, NULL);
  free(server_content);
}

void post_with_headers(test_t *test) {
  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./post_with_headers", NULL);
  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  char *server_content = READ_FILE("./server-output.txt");
  char *expected_server_content = "HEADER1:value1|HEADER2:value2";
  ASSERT_EQ_STR(expected_server_content, server_content, NULL);
  free(server_content);
}

void receive_headers(test_t *test) {
  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./receive_headers", NULL);
  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  char *client_content = READ_FILE("./client-output.txt");
  char *expected_client_content = "STATUS:3|Server:Sonic|Content-Type:text/"
                                  "plain|Content-Length:11||BODY:Hello world";
  ASSERT_EQ_STR(expected_client_content, client_content, NULL);
  free(client_content);
}

void response_callback(test_t *test) {
  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./response_callback", NULL);
  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  bool equal = FILES_EQUAL("./downloaded_file.bin", "./large_test_file.bin");

  ASSERT_EQ_BOOL(equal, true, NULL);
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
  test_t *get_200_test = ADD_REPEAT_TEST(get_200, 20);
  ADD_SETUP(get_200_test, setup);
  ADD_TEARDOWN(get_200_test, teardown);

  test_t *get_404_test = ADD_REPEAT_TEST(get_404, 20);
  ADD_SETUP(get_404_test, setup);
  ADD_TEARDOWN(get_404_test, teardown);

  test_t *post_with_body_test = ADD_REPEAT_TEST(post_with_body, 20);
  ADD_SETUP(post_with_body_test, setup);
  ADD_TEARDOWN(post_with_body_test, teardown);

  test_t *get_with_headers_test = ADD_REPEAT_TEST(post_with_headers, 20);
  ADD_SETUP(get_with_headers_test, setup);
  ADD_TEARDOWN(get_with_headers_test, teardown);

  test_t *receive_headers_test = ADD_REPEAT_TEST(receive_headers, 20);
  ADD_SETUP(receive_headers_test, setup);
  ADD_TEARDOWN(receive_headers_test, teardown);

  test_t *response_callback_test = ADD_TEST(response_callback);
  ADD_SETUP(response_callback_test, setup);
  ADD_TEARDOWN(response_callback_test, teardown);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(FUNCTIONAL);

  ADD_SUITE(test_suite);

  CAMEL_END();
}
