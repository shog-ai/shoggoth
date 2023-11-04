#define _GNU_SOURCE

#include "../../../../src/include/camel.h"
#include "../../../../src/include/sonic.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void test_api_get_dht(test_t *test) {
  sonic_response_t *resp = sonic_get("http://127.0.0.1:6969/api/get_manifest");

  ASSERT(!resp->failed, "");

  ASSERT_EQ((int)resp->response_body_size, 584, NULL);

  if (resp->response_body_size > 0) {
    free(resp->response_body);
  }

  sonic_free_response(resp);
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
    printf("%s\n", node_output);
    free(node_output);
  }

  USE_SETUP(process_t);

  KILL_PROCESS(SETUP_VALUE, SIGINT);
  int node_status = AWAIT_PROCESS(SETUP_VALUE);

  if (0 != node_status) {
    printf("Node exit status was not 0 \n");

    printf("Dumping node output:\n");
    char *node_output = READ_FILE("./node-output.txt");
    printf("%s\n", node_output);
    free(node_output);
  }

  sleep(2);

  FREE_SETUP();
}

void test_suite(suite_t *suite) {
  test_t *api_get_dht_test = ADD_REPEAT_TEST(test_api_get_dht, 4);
  ADD_SETUP(api_get_dht_test, setup_start_node);
  ADD_TEARDOWN(api_get_dht_test, teardown_stop_node);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(FUNCTIONAL);

  ADD_SUITE(test_suite);

  CAMEL_END();
}
