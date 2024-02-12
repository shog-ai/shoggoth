#include "../../../../include/camel.h"

#include <stdlib.h>
#include <unistd.h>

void get_200(test_t *test) {
  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./get_200", NULL);
  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  char *expected_content = "STATUS:3|BODY:{\"count\":2274744,\"name\":\"john\","
                           "\"gender\":\"male\",\"probability\":1.0}";
  char *content = READ_FILE("./client-output.txt");
  ASSERT_EQ_STR(expected_content, content, NULL);
  free(content);
}

void with_subdomain(test_t *test) {
  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./with_subdomain", NULL);
  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  char *expected_content = "STATUS:3|BODY:{\"count\":2274744,\"name\":\"john\","
                           "\"gender\":\"male\",\"probability\":1.0}";
  char *content = READ_FILE("./client-output.txt");
  ASSERT_EQ_STR(expected_content, content, NULL);
  free(content);
}

void ip_and_port(test_t *test) {
  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./ip_and_port", NULL);
  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  char *expected_content = "STATUS:3|BODY:{\"count\":2274744,\"name\":\"john\","
                           "\"gender\":\"male\",\"probability\":1.0}";
  char *content = READ_FILE("./client-output.txt");
  ASSERT_EQ_STR(expected_content, content, NULL);
  free(content);
}

void no_https(test_t *test) {
  process_t *client_process =
      RUN_EXECUTABLE("./client-output.txt", "./no_https", NULL);
  int client_status = AWAIT_PROCESS(client_process);
  ASSERT_EQ(0, client_status, NULL);

  free(client_process);

  char *expected_content =
      "STATUS:11|BODY:<html>\r\n"
      "<head><title>301 Moved Permanently</title></head>\r\n"
      "<body>\r\n"
      "<center><h1>301 Moved Permanently</h1></center>\r\n"
      "<hr><center>nginx</center>\r\n"
      "</body>\r\n"
      "</html>\r\n";

  char *content = READ_FILE("./client-output.txt");

  ASSERT_EQ_STR(expected_content, content, NULL);
  free(content);
}

void teardown(test_t *test) {
  if (test->test_failed) {
    printf("Dumping client output:\n");
    char *client_output = READ_FILE("./client-output.txt");
    printf("%s\n", client_output);
    free(client_output);
  }
}

VALUE_T setup() {
  //

  return NULL;
}

void test_suite(suite_t *suite) {
  test_t *get_200_test = ADD_REPEAT_TEST(get_200, 2);
  ADD_SETUP(get_200_test, setup);
  ADD_TEARDOWN(get_200_test, teardown);

  test_t *with_subdomain_test = ADD_REPEAT_TEST(with_subdomain, 2);
  ADD_SETUP(with_subdomain_test, setup);
  ADD_TEARDOWN(with_subdomain_test, teardown);

  test_t *ip_and_port_test = ADD_REPEAT_TEST(ip_and_port, 2);
  ADD_SETUP(ip_and_port_test, setup);
  ADD_TEARDOWN(ip_and_port_test, teardown);

  test_t *no_https_test = ADD_REPEAT_TEST(no_https, 2);
  ADD_SETUP(no_https_test, setup);
  ADD_TEARDOWN(no_https_test, teardown);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(FUNCTIONAL);

  ADD_SUITE(test_suite);

  CAMEL_END();
}
