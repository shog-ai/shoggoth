#include "../../../../include/camel.h"

#include "../../../../sonic.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void serve_200(test_t *test) {
  sonic_request_t *req =
      sonic_new_request(METHOD_GET, "http://127.0.0.1:5000/serve_200");

  sonic_response_t *resp = sonic_send_request(req);

  ASSERT_EQ_BOOL(false, resp->failed, NULL);

  ASSERT_EQ(resp->status, 3, NULL);

  char body[256];
  strncpy(body, resp->response_body, resp->response_body_size);
  body[resp->response_body_size] = '\0';

  ASSERT_EQ_STR(body, "Hello world", NULL);

  free(resp->response_body);

  sonic_free_request(req);
  sonic_free_response(resp);
}

void serve_404(test_t *test) {
  sonic_request_t *req = sonic_new_request(
      METHOD_GET, "http://127.0.0.1:5000/path_that_doesnt_exist");

  sonic_response_t *resp = sonic_send_request(req);

  ASSERT_EQ_BOOL(false, resp->failed, NULL);

  ASSERT_EQ(resp->status, 21, NULL);

  free(resp->response_body);

  sonic_free_request(req);
  sonic_free_response(resp);
}

// send a malformed request
void serve_400(test_t *test) {
  // TODO: try to send a malformed request and receive a "400 Bad requst"
}

void serve_with_headers(test_t *test) {
  sonic_request_t *req =
      sonic_new_request(METHOD_GET, "http://127.0.0.1:5000/serve_200");

  sonic_response_t *resp = sonic_send_request(req);

  ASSERT_EQ_BOOL(false, resp->failed, NULL);

  ASSERT_EQ(resp->status, 3, NULL);

  char *server_header =
      sonic_get_header_value(resp->headers, resp->headers_count, "Server");
  ASSERT_NOT_NULL(server_header, NULL);
  ASSERT_EQ_STR(server_header, "Sonic", NULL);

  free(resp->response_body);

  sonic_free_request(req);
  sonic_free_response(resp);
}

const char *downloaded_file_path = "./downloaded_file.bin";

void response_callback(char *data, u64 size, void *user_pointer) {
  FILE *file = fopen(downloaded_file_path, "a");
  if (file == NULL) {
    perror("Error opening file");
    exit(1);
  }

  size_t bytes_written = fwrite(data, 1, size, file);
  if (bytes_written != size) {
    perror("Error writing to file");
    exit(1);
  }

  fclose(file);
}

void serve_large_file(test_t *test) {
  FILE *file = fopen(downloaded_file_path, "w");
  if (file == NULL) {
    perror("Error opening file");
    exit(1);
  }
  fclose(file);

  sonic_request_t *req =
      sonic_new_request(METHOD_GET, "http://127.0.0.1:5000/large_file");

  sonic_request_set_response_callback(req, response_callback, NULL);

  sonic_response_t *resp = sonic_send_request(req);

  ASSERT_EQ_BOOL(false, resp->failed, NULL);
  ASSERT_EQ(resp->status, 3, NULL);

  bool equal = FILES_EQUAL("./downloaded_file.bin", "./large_test_file.bin");
  ASSERT_EQ_BOOL(equal, true, NULL);

  sonic_free_request(req);
  sonic_free_response(resp);
}

VALUE_T setup() {
  INIT_SETUP(process_t);
  free(SETUP_VALUE);

  process_t *server_process =
      RUN_EXECUTABLE("./server-output.txt", "./server", NULL);

  SETUP_VALUE = server_process;

  usleep(1000000);

  SETUP_RETURN();
}

void teardown(test_t *test) {
  if (test->test_failed) {
    printf("Dumping server output:\n");
    char *server_output = READ_FILE("./server-output.txt");
    printf("%s\n", server_output);
    free(server_output);
  }

  USE_SETUP(process_t);

  KILL_PROCESS(SETUP_VALUE, SIGTERM);
  int server_status = AWAIT_PROCESS(SETUP_VALUE);
  ASSERT_EQ(0, server_status, NULL);

  FREE_SETUP();
}

void test_suite(suite_t *suite) {
  test_t *serve_200_test = ADD_REPEAT_TEST(serve_200, 10);
  ADD_SETUP(serve_200_test, setup);
  ADD_TEARDOWN(serve_200_test, teardown);

  test_t *serve_404_test = ADD_REPEAT_TEST(serve_404, 10);
  ADD_SETUP(serve_404_test, setup);
  ADD_TEARDOWN(serve_404_test, teardown);

  test_t *serve_400_test = ADD_REPEAT_TEST(serve_400, 10);
  ADD_SETUP(serve_400_test, setup);
  ADD_TEARDOWN(serve_400_test, teardown);

  test_t *serve_with_headers_test = ADD_REPEAT_TEST(serve_with_headers, 10);
  ADD_SETUP(serve_with_headers_test, setup);
  ADD_TEARDOWN(serve_with_headers_test, teardown);

  test_t *serve_large_file_test = ADD_REPEAT_TEST(serve_large_file, 2);
  ADD_SETUP(serve_large_file_test, setup);
  ADD_TEARDOWN(serve_large_file_test, teardown);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(FUNCTIONAL);

  ADD_SUITE(test_suite);

  CAMEL_END();
}
