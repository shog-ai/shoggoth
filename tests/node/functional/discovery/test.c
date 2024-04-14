#define _GNU_SOURCE

#include "../../../../src/include/camel.h"
#include "../../../../src/include/sonic.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void test_discovery(test_t *test) {
  sonic_response_t *resp = sonic_get("http://127.0.0.1:6969/api/get_dht");
  ASSERT(!resp->failed, "request 1 failed");
  ASSERT_EQ((int)resp->response_body_size, 1156, NULL);
  if (resp->response_body_size > 0) {
    free(resp->response_body);
  }
  sonic_free_response(resp);

  resp = sonic_get("http://127.0.0.1:4969/api/get_dht");
  ASSERT(!resp->failed, "request 2 failed");
  ASSERT_EQ((int)resp->response_body_size, 1156, NULL);
  if (resp->response_body_size > 0) {
    free(resp->response_body);
  }
  sonic_free_response(resp);

  resp = sonic_get("http://127.0.0.1:5969/api/get_dht");
  ASSERT(!resp->failed, "request 3 failed");
  ASSERT_EQ((int)resp->response_body_size, 1156, NULL);
  if (resp->response_body_size > 0) {
    free(resp->response_body);
  }
  sonic_free_response(resp);
}

typedef struct {
  process_t **processes;
  u64 count;
} process_group_t;

VALUE_T setup_start_triple_nodes() {
  INIT_SETUP(process_group_t);

  char working_dir[1024];
  getcwd(working_dir, sizeof(working_dir));

  char node_path[1054];

  sprintf(node_path, "%s/node2", working_dir);
  process_t *node_process2 = RUN_EXECUTABLE(
      "./node-output2.txt", "./shog", "run", "-r", node_path, NULL);

  sleep(5);

  sprintf(node_path, "%s/node1", working_dir);
  process_t *node_process1 = RUN_EXECUTABLE(
      "./node-output1.txt", "./shog", "run", "-r", node_path, NULL);

  sprintf(node_path, "%s/node3", working_dir);
  process_t *node_process3 = RUN_EXECUTABLE(
      "./node-output3.txt", "./shog", "run", "-r", node_path, NULL);

  SETUP_VALUE->processes = malloc(3 * sizeof(process_t *));
  SETUP_VALUE->count = 3;

  SETUP_VALUE->processes[0] = node_process1;
  SETUP_VALUE->processes[1] = node_process2;
  SETUP_VALUE->processes[2] = node_process3;

  sleep(20);

  SETUP_RETURN();
}

void teardown_stop_triple_nodes(test_t *test) {
  if (test->test_failed) {
    printf("Dumping node output:\n");

    char *node_output = READ_FILE("./node-output1.txt");
    printf("%s\n", node_output);
    free(node_output);

    node_output = READ_FILE("./node-output2.txt");
    printf("%s\n", node_output);
    free(node_output);

    node_output = READ_FILE("./node-output3.txt");
    printf("%s\n", node_output);
    free(node_output);
  }

  USE_SETUP(process_group_t);

  for (u64 i = 0; i < SETUP_VALUE->count; i++) {
    KILL_PROCESS(SETUP_VALUE->processes[i], SIGINT);
    int node_status = AWAIT_PROCESS(SETUP_VALUE->processes[i]);

    if (node_status != 0) {
      test->test_failed = true;
    }

    free(SETUP_VALUE->processes[i]);
  }

  sleep(2);

  free(SETUP_VALUE->processes);

  FREE_SETUP();
}

void test_suite(suite_t *suite) {
  test_t *discovery_test = ADD_REPEAT_TEST(test_discovery, 4);
  ADD_SETUP(discovery_test, setup_start_triple_nodes);
  ADD_TEARDOWN(discovery_test, teardown_stop_triple_nodes);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(FUNCTIONAL);

  ADD_SUITE(test_suite);

  CAMEL_END();
}
