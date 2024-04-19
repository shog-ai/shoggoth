#include "../../../include/camel.h"

#include "../../../include/client.h"

#include <netlibc.h>

#include <stdlib.h>

void test_new_client_request(test_t *test) {
  sonic_client_request_t *req =
      new_client_request(METHOD_GET, "https://shoggoth.network/");

  ASSERT_EQ_STR(req->host, "shoggoth.network", NULL);
  ASSERT_EQ(req->port, 443, NULL);
  ASSERT_EQ_STR("/", req->path, NULL);
  ASSERT_EQ(req->method, METHOD_GET, NULL);

  ASSERT_NULL(req->request_body, NULL);
  ASSERT_EQ((int)req->request_body_size, 0, NULL);

  ASSERT_NULL(req->headers, NULL);
  ASSERT_EQ((int)req->headers_count, 0, NULL);

  client_free_request(req);
}

void test_invalid_scheme(test_t *test) {
  sonic_client_request_t *req =
      new_client_request(METHOD_GET, "hps://shoggoth.network/");

  ASSERT_NULL(req, NULL);

  char *error = sonic_get_error();
  ASSERT_EQ_STR(error, "invalid scheme", NULL);
}

void test_request_set_body(test_t *test) {
  sonic_client_request_t *req =
      sonic_new_request(METHOD_GET, "https://shoggoth.network/");

  client_request_set_body(req, "foo", strlen("foo"));

  ASSERT_NOT_NULL(req->request_body, NULL);

  ASSERT_EQ((int)req->request_body_size, 3, NULL);

  client_free_request(req);
}

void test_new_client_response(test_t *test) {
  sonic_client_response_t *resp = new_client_response();

  ASSERT_NULL(resp->response_body, NULL);
  ASSERT_EQ((int)resp->response_body_size, 0, NULL);

  ASSERT_NULL(resp->headers, NULL);
  ASSERT_EQ((int)resp->headers_count, 0, NULL);

  ASSERT_EQ_BOOL(resp->failed, false, NULL);
  ASSERT_NULL(resp->error, NULL);

  client_free_response(resp);
}

void test_http_extract_status_code(test_t *test) {
  char *head = "HTTP/1.1 200 OK\r\n"
               "Server: Shoggoth\r\n"
               "Content-Type: application/octet-stream\r\n"
               "Content-Length: 0\r\n\r\n";

  u16 status_code = 0;
  int failed = http_extract_status_code(head, &status_code);

  ASSERT_EQ(status_code, 200, NULL);
}

void test_http_parse_response_head(test_t *test) {
  char *head = "HTTP/1.1 200 OK\r\n"
               "Server: Shoggoth\r\n"
               "Content-Type: application/octet-stream\r\n"
               "Content-Length: 0\r\n\r\n";

  sonic_client_response_t *resp = new_client_response();

  http_parse_response_head(resp, head);

  ASSERT_EQ((int)resp->headers_count, 3, NULL);

  client_free_response(resp);
}

void test_http_fail_response(test_t *test) {
  sonic_client_response_t *resp = fail_response("failed");

  ASSERT_EQ_BOOL(resp->failed, true, NULL);
  ASSERT_EQ_STR(resp->error, "failed", NULL);

  client_free_response(resp);
}

void test_request_add_header(test_t *test) {
  sonic_client_request_t *req =
      new_client_request(METHOD_GET, "http://shoggoth.network");

  client_request_add_header(req, "key", "value");

  client_free_request(req);
}

void test_suite(suite_t *suite) {
  ADD_TEST(test_new_client_request);
  ADD_TEST(test_invalid_scheme);
  ADD_TEST(test_request_set_body);
  ADD_TEST(test_request_set_body);
  ADD_TEST(test_http_extract_status_code);
  ADD_TEST(test_http_parse_response_head);
  ADD_TEST(test_http_fail_response);
  ADD_TEST(test_request_add_header);
}

int main(int argc, char **argv) {
  NETLIBC_INIT();
  
  CAMEL_BEGIN(UNIT);

  ADD_SUITE(test_suite);

  CAMEL_END();
}
