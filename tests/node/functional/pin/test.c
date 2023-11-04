#define _GNU_SOURCE

#include "../../../../src/include/camel.h"
#include "../../../../src/include/sonic.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void test_pin(test_t *test) {
  sonic_response_t *resp = sonic_get("http://127.0.0.1:6969/api/get_dht");
  ASSERT(!resp->failed, "request 1 failed");
  ASSERT_EQ((int)resp->response_body_size, 935, NULL);
  if (resp->response_body_size > 0) {
    free(resp->response_body);
  }
  sonic_free_response(resp);

  resp = sonic_get("http://127.0.0.1:3969/api/get_dht");
  ASSERT(!resp->failed, "request 2 failed");
  ASSERT_EQ((int)resp->response_body_size, 935, NULL);
  if (resp->response_body_size > 0) {
    free(resp->response_body);
  }
  sonic_free_response(resp);

  resp = sonic_get("http://127.0.0.1:5969/api/get_dht");
  ASSERT(!resp->failed, "request 3 failed");
  ASSERT_EQ((int)resp->response_body_size, 935, NULL);
  if (resp->response_body_size > 0) {
    free(resp->response_body);
  }
  sonic_free_response(resp);

  //

  char working_dir[1024];
  getcwd(working_dir, sizeof(working_dir));

  char runtime_path[1054];
  sprintf(runtime_path, "%s/client", working_dir);

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

  bool equal =
      FILES_EQUAL("./client/client/tmp/shoggoth-profile.zip",
                  "./node2/node/pins/SHOG2b93e6380b074138a155f19ba9d79381.zip");
  ASSERT(equal, "files should be equal");

  //

  // 20 seconds to wait for other nodes to obtain pin
  sleep(20);

  // confirm all nodes have obtained the new pin

  resp = sonic_get("http://127.0.0.1:6969/api/get_pins");
  ASSERT(!resp->failed, "request 1 failed");
  // printf("BODY: %.*s\n", (int)resp->response_body_size, resp->response_body);
  ASSERT_EQ((int)resp->response_body_size, 40, NULL);
  if (resp->response_body_size > 0) {
    free(resp->response_body);
  }
  sonic_free_response(resp);

  resp = sonic_get("http://127.0.0.1:3969/api/get_pins");
  ASSERT(!resp->failed, "request 2 failed");
  ASSERT_EQ((int)resp->response_body_size, 40, NULL);
  if (resp->response_body_size > 0) {
    free(resp->response_body);
  }
  sonic_free_response(resp);

  resp = sonic_get("http://127.0.0.1:5969/api/get_pins");
  ASSERT(!resp->failed, "request 3 failed");
  ASSERT_EQ((int)resp->response_body_size, 40, NULL);
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
      "./node-output2.txt", "./shog", "node", "run", "-r", node_path, NULL);

  sleep(5);

  sprintf(node_path, "%s/node1", working_dir);
  process_t *node_process1 = RUN_EXECUTABLE(
      "./node-output1.txt", "./shog", "node", "run", "-r", node_path, NULL);

  sprintf(node_path, "%s/node3", working_dir);
  process_t *node_process3 = RUN_EXECUTABLE(
      "./node-output3.txt", "./shog", "node", "run", "-r", node_path, NULL);

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

    printf("Dumping client output:\n");

    char *client_output = READ_FILE("./client-output.txt");
    printf("%s\n", client_output);
    free(client_output);
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
  test_t *discovery_test = ADD_REPEAT_TEST(test_pin, 4);
  ADD_SETUP(discovery_test, setup_start_triple_nodes);
  ADD_TEARDOWN(discovery_test, teardown_stop_triple_nodes);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(INTEGRATION);

  ADD_SUITE(test_suite);

  CAMEL_END();
}
