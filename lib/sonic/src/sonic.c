/**** sonic.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Sonic HTTP library, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../include/client.h"
#include "../include/server.h"
#include "../include/utils.h"

#include "../sonic.h"

sonic_client_error_t __sonic_client_errno = CLIENT_ERROR_NONE;

sonic_wildcard_segments_t
sonic_get_path_wildcard_segments(sonic_server_request_t *req) {
  return server_get_path_wildcard_segments(req);
}

void sonic_free_path_wildcard_segments(sonic_wildcard_segments_t segments) {
  server_free_path_wildcard_segments(segments);
}

char *sonic_get_path_segment(sonic_server_request_t *req, char *segment_name) {
  return server_get_path_segment(req, segment_name);
}

void client_set_errno(sonic_client_error_t error) {
  //

  __sonic_client_errno = error;
}

char *sonic_get_error() {
  if (__sonic_client_errno == CLIENT_ERROR_INVALID_SCHEME) {
    return "invalid scheme";
  } else {
    return "SONIC CLIENT ERRNO UNHANDLED ERROR NUMBER";
  }
}

void sonic_add_directory_route(sonic_server_t *server, char *path,
                               char *directory_path) {
  return server_add_directory_route(server, path, directory_path);
}

void sonic_stop_server(sonic_server_t *server) { return stop_server(server); }

char *sonic_status_to_string(sonic_status_t status_code) {
  return utils_status_code_to_string(status_code);
}

char *sonic_get_header_value(sonic_header_t *headers, u64 headers_count,
                             char *key) {
  return utils_get_header_value(headers, headers_count, key);
}

sonic_client_response_t *sonic_get(char *url) {
  sonic_client_request_t *req = new_client_request(METHOD_GET, url);

  sonic_client_response_t *resp = client_send_request(req);

  client_free_request(req);

  return resp;
}

sonic_client_request_t *sonic_new_request(sonic_method_t method, char *url) {
  return new_client_request(method, url);
}

void sonic_request_set_response_callback(
    sonic_request_t *req, response_callback_func_t callback_func, void *user_pointer) {
  client_set_response_callback(req, callback_func, user_pointer);
}

void sonic_free_request(sonic_request_t *req) { client_free_request(req); }

void sonic_free_response(sonic_response_t *resp) { client_free_response(resp); }

/****U
 * sets the body of a request
 *
 ****/
void sonic_set_body(sonic_client_request_t *req, char *request_body,
                    u64 request_body_size) {
  return client_request_set_body(req, request_body, request_body_size);
}

void sonic_set_redirect_policy(sonic_client_request_t *req,
                               sonic_redirect_policy_t policy) {
  return client_set_redirect_policy(req, policy);
}

sonic_client_response_t *sonic_send_request(sonic_client_request_t *req) {
  return client_send_request(req);
}

void sonic_add_header(sonic_client_request_t *req, char *key, char *value) {
  return client_request_add_header(req, key, value);
}

void sonic_free_server_response(sonic_server_response_t *resp) {
  return free_server_response(resp);
}

void sonic_send_response(sonic_server_request_t *req,
                         sonic_server_response_t *resp) {
  return server_send_response(req, resp);
}

void sonic_add_file_route(sonic_server_t *server, char *path,
                          sonic_method_t method, char *file_path,
                          sonic_content_type_t content_type) {

  return server_add_file_route(server, path, method, file_path, content_type);
}

sonic_route_t *sonic_add_route(sonic_server_t *server, char *path,
                               sonic_method_t method,
                               sonic_route_func_t route_func) {

  return server_add_route(server, path, method, route_func);
}

void sonic_add_middleware(sonic_route_t *route,
                          sonic_middleware_func_t middleware_func,
                          void *user_pointer) {

  return server_add_route_middleware(route, middleware_func, user_pointer);
}

void sonic_add_server_middleware(sonic_server_t *server,
                                 sonic_middleware_func_t middleware_func,
                                 void *user_pointer) {

  return server_add_server_middleware(server, middleware_func, user_pointer);
}

void sonic_add_rate_limiter_middleware(sonic_route_t *route, u64 requests_count,
                                       u64 duration, char *message) {

  return server_add_rate_limiter_middleware(route, requests_count, duration,
                                            message);
}

void sonic_add_server_rate_limiter_middleware(sonic_server_t *server,
                                              u64 requests_count, u64 duration,
                                              char *message) {

  return server_add_server_rate_limiter_middleware(server, requests_count,
                                                   duration, message);
}

sonic_middleware_response_t *
sonic_new_middleware_response(bool should_respond,
                              sonic_server_response_t *response) {

  return new_server_middleware_response(should_respond, response);
}

int sonic_start_server(sonic_server_t *http_server) {
  return start_server(http_server);
}

sonic_server_t *sonic_new_server(char *host, u16 port) {
  return new_server(host, port);
}

sonic_server_response_t *sonic_new_response(sonic_status_t status,
                                            sonic_content_type_t content_type) {
  return new_server_response(status, content_type);
}

void sonic_response_set_body(sonic_server_response_t *resp, char *response_body,
                             u64 response_body_size) {
  return server_response_set_body(resp, response_body, response_body_size);
}

void sonic_response_add_header(sonic_server_response_t *resp, char *key,
                               char *value) {
  return server_response_add_header(resp, key, value);
}
