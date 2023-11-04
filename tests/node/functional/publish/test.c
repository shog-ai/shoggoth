#define _GNU_SOURCE

#include "../../../../src/include/camel.h"
#include "../../../../src/include/sonic.h"

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void test_publish(test_t *test) {
  sonic_response_t *resp = sonic_get("http://127.0.0.1:6969/api/get_manifest");
  ASSERT(!resp->failed, "");
  ASSERT_EQ((int)resp->response_body_size, 584, NULL);
  if (resp->response_body_size > 0) {
    free(resp->response_body);
  }
  sonic_free_response(resp);

  char working_dir[1024];
  getcwd(working_dir, sizeof(working_dir));

  char runtime_path[1054];
  sprintf(runtime_path, "%s/runtime", working_dir);

  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./shog", "client", "init", "-r",
                     runtime_path, NULL);

  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  chdir("./shoggoth-profile");
  client_process = RUN_EXECUTABLE("../client-output.txt", "../shog", "client",
                                  "publish", "-r", runtime_path, NULL);
  chdir(working_dir);

  client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  bool equal = FILES_EQUAL(
      "./runtime/client/tmp/shoggoth-profile.zip",
      "./runtime/node/pins/SHOG2b93e6380b074138a155f19ba9d79381.zip");

  ASSERT(equal, "files should be equal");
}

void test_clone(test_t *test) {
  sonic_response_t *resp = sonic_get("http://127.0.0.1:6969/api/get_manifest");
  ASSERT(!resp->failed, "");
  ASSERT_EQ((int)resp->response_body_size, 584, NULL);
  if (resp->response_body_size > 0) {
    free(resp->response_body);
  }
  sonic_free_response(resp);

  char working_dir[1024];
  getcwd(working_dir, sizeof(working_dir));

  char runtime_path[1054];
  sprintf(runtime_path, "%s/runtime", working_dir);

  process_t *client_process = RUN_EXECUTABLE(
      "./client-output.txt", "./shog", "client", "clone",
      "SHOG2b93e6380b074138a155f19ba9d79381", "-r", runtime_path, NULL);

  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  bool equal = FILES_EQUAL(
      "./shoggoth-profile/.shoggoth/manifest.json",
      "./SHOG2b93e6380b074138a155f19ba9d79381/.shoggoth/manifest.json");

  ASSERT(equal, "files should be equal");
}

VALUE_T setup_start_node() {
  INIT_SETUP(process_t);
  free(SETUP_VALUE);

  char working_dir[1024];
  getcwd(working_dir, sizeof(working_dir));

  char runtime_path[1054];
  sprintf(runtime_path, "%s/runtime", working_dir);

  process_t *node_process = RUN_EXECUTABLE(
      "./node-output.txt", "./shog", "node", "run", "-r", runtime_path, NULL);

  SETUP_VALUE = node_process;

  sleep(5);

  SETUP_RETURN();
}

void teardown_stop_node(test_t *test) {
  if (test->test_failed) {
    printf("Dumping node output:\n");
    char *node_output = READ_FILE("./node-output.txt");
    assert(node_output != NULL);
    printf("%s\n", node_output);
    free(node_output);

    printf("Dumping client output:\n");
    char *client_output = READ_FILE("./client-output.txt");
    assert(client_output != NULL);
    printf("%s\n", client_output);
    free(client_output);
  }

  USE_SETUP(process_t);

  KILL_PROCESS(SETUP_VALUE, SIGINT);
  int node_status = AWAIT_PROCESS(SETUP_VALUE);
  ASSERT_EQ(0, node_status, NULL);

  sleep(2);

  FREE_SETUP();
}

void test_suite(suite_t *suite) {
  test_t *publish_test = ADD_REPEAT_TEST(test_publish, 4);
  ADD_SETUP(publish_test, setup_start_node);
  ADD_TEARDOWN(publish_test, teardown_stop_node);

  test_t *clone_test = ADD_REPEAT_TEST(test_clone, 4);
  ADD_SETUP(clone_test, setup_start_node);
  ADD_TEARDOWN(clone_test, teardown_stop_node);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(INTEGRATION);

  ADD_SUITE(test_suite);

  CAMEL_END();
}
