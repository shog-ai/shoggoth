/**** sonic.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Sonic HTTP library, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SONIC_H
#define SONIC_H

#include <netlibc.h>
#include <netlibc/error.h>
#include <netlibc/fs.h>
#include <netlibc/log.h>
#include <netlibc/string.h>

typedef enum {
  CLIENT_ERROR_NONE,
  CLIENT_ERROR_INVALID_SCHEME
} sonic_client_error_t;

extern sonic_client_error_t __sonic_client_errno;

void client_set_errno(sonic_client_error_t error);

char *sonic_get_error();

typedef enum {
  METHOD_NULL,
  METHOD_GET,
  METHOD_HEAD,
  METHOD_POST,
  METHOD_PUT,
  METHOD_DELETE,
  METHOD_CONNECT,
  METHOD_OPTIONS,
  METHOD_TRACE,
} sonic_method_t;

typedef enum {
  SCHEME_HTTP,
  SCHEME_HTTPS,
} sonic_scheme_t;

typedef enum {
  STATUS_NULL,
  STATUS_100,
  STATUS_101,
  STATUS_200,
  STATUS_201,
  STATUS_202,
  STATUS_203,
  STATUS_204,
  STATUS_205,
  STATUS_206,
  STATUS_300,
  STATUS_301,
  STATUS_302,
  STATUS_303,
  STATUS_304,
  STATUS_305,
  STATUS_307,
  STATUS_400,
  STATUS_401,
  STATUS_402,
  STATUS_403,
  STATUS_404,
  STATUS_405,
  STATUS_406,
  STATUS_407,
  STATUS_408,
  STATUS_409,
  STATUS_410,
  STATUS_411,
  STATUS_412,
  STATUS_413,
  STATUS_414,
  STATUS_415,
  STATUS_416,
  STATUS_417,
  STATUS_426,
  STATUS_429,
  STATUS_500,
  STATUS_501,
  STATUS_502,
  STATUS_503,
  STATUS_504,
  STATUS_505,
} sonic_status_t;

typedef enum {
  MIME_AUDIO_AAC,
  MIME_AUDIO_MPEG,
  MIME_AUDIO_WAV,
  MIME_AUDIO_WEBM,

  MIME_VIDEO_MP4,
  MIME_VIDEO_3GPP,
  MIME_VIDEO_WEBM,
  MIME_VIDEO_MPEG,
  MIME_VIDEO_X_MSVIDEO,

  MIME_APPLICATION_ZIP,
  MIME_APPLICATION_XHTML,
  MIME_APPLICATION_XML,
  MIME_APPLICATION_X_TAR,
  MIME_APPLICATION_PDF,
  MIME_APPLICATION_JAVA_ARCHIVE,
  MIME_APPLICATION_JSON,
  MIME_APPLICATION_OCTET_STREAM,
  MIME_APPLICATION_MSWORD,
  MIME_APPLICATION_MSWORDX,
  MIME_APPLICATION_GZIP,

  MIME_TEXT_XML,
  MIME_TEXT_PLAIN,
  MIME_TEXT_CSV,
  MIME_TEXT_HTML,
  MIME_TEXT_CSS,
  MIME_TEXT_JAVASCRIPT,

  MIME_IMAGE_GIF,
  MIME_IMAGE_WEBP,
  MIME_IMAGE_ICO,
  MIME_IMAGE_BMP,
  MIME_IMAGE_PNG,
  MIME_IMAGE_JPEG,
  MIME_IMAGE_SVG,
} sonic_content_type_t;

typedef struct {
  char *key;
  char *value;
} sonic_header_t;

char *sonic_get_header_value(sonic_header_t *headers, u64 headers_count,
                             char *key);

// CLIENT
// ===================================================================================================================

typedef struct {
  sonic_status_t status;

  sonic_header_t *headers;
  u64 headers_count;

  char *response_body;
  u64 response_body_size;

  bool failed;
  char *error;

  bool should_redirect;
  char *redirect_location;
} sonic_client_response_t;

typedef void (*response_callback_func_t)(char *data, u64 size,
                                         void *user_pointer);

typedef struct {
  u64 max_hops;
  bool no_redirect;
} sonic_redirect_policy_t;

typedef struct {
  char *host;
  char *domain_name;
  u16 port;
  char *path;
  sonic_method_t method;
  sonic_scheme_t scheme;
  char *request_body;
  u64 request_body_size;

  sonic_header_t *headers;
  u64 headers_count;

  bool use_response_callback;
  response_callback_func_t response_callback_func;
  void *response_callback_user_pointer;

  sonic_redirect_policy_t redirect_policy;
  u64 redirect_hops_count;
} sonic_client_request_t;

typedef sonic_client_response_t sonic_response_t;

typedef sonic_client_request_t sonic_request_t;

sonic_response_t *sonic_get(char *url);

sonic_request_t *sonic_new_request(sonic_method_t method, char *url);

void sonic_free_request(sonic_request_t *req);

void sonic_free_response(sonic_response_t *resp);

void sonic_request_set_response_callback(sonic_request_t *req,
                                         response_callback_func_t callback_func,
                                         void *user_pointer);

sonic_response_t *sonic_send_request(sonic_request_t *req);

void sonic_set_redirect_policy(sonic_client_request_t *req,
                               sonic_redirect_policy_t policy);

void sonic_add_header(sonic_client_request_t *req, char *key, char *value);

void sonic_set_body(sonic_client_request_t *req, char *request_body,
                    u64 request_body_size);

result_t sonic_status_to_string(sonic_status_t status_code);

// ===================================================================================================================

// SERVER
// ===================================================================================================================
typedef struct {
  sonic_status_t status;
  sonic_content_type_t content_type;
  sonic_header_t *headers;
  u64 headers_count;

  char *response_body;
  u64 response_body_size;

  bool is_file;
  char *file_path;
} sonic_server_response_t;

typedef struct {
  bool should_respond;
  sonic_server_response_t *middleware_response;
} sonic_middleware_response_t;

typedef struct {
  char *value;
  bool is_generic;
  bool is_wildcard;
} sonic_path_segment_t;

typedef struct {
  char **segments;
  u64 count;
} sonic_wildcard_segments_t;

typedef struct {
  sonic_path_segment_t *segments;
  u64 segments_count;
  bool has_wildcard;
  u64 wildcard_index;
} sonic_path_t;

typedef struct {
  struct SONIC_ROUTE *matched_route;

  char *client_ip;
  u64 client_port;

  int client_sock;

  sonic_method_t method;
  sonic_path_t *path;
  sonic_header_t *headers;
  u64 headers_count;

  char *request_body;
  u64 request_body_size;
} sonic_server_request_t;

typedef void (*sonic_route_func_t)(sonic_server_request_t *req);

typedef void (*sonic_file_route_func_t)(sonic_server_request_t *req,
                                        char *file_path,
                                        sonic_content_type_t content_type);

typedef sonic_middleware_response_t *(*sonic_middleware_func_t)(
    sonic_server_request_t *req, void *user_pointer);

typedef struct {
  sonic_middleware_func_t middleware_func;
  void *user_pointer;
} sonic_route_middleware_t;

typedef struct SONIC_ROUTE {
  sonic_method_t method;
  sonic_path_t *path;
  sonic_route_func_t route_func;

  bool is_file_route;
  sonic_file_route_func_t file_route_func;
  char *file_route_path;
  sonic_content_type_t file_route_content_type;

  sonic_route_middleware_t **middlewares;
  u64 middlewares_count;
} sonic_route_t;

typedef struct {
  char *host;
  u16 port;
  sonic_route_t **routes;
  u64 routes_count;
  bool should_exit;
  int server_socket;

  sonic_route_middleware_t **middlewares;
  u64 middlewares_count;
} sonic_server_t;

result_t sonic_send_response(sonic_server_request_t *req,
                             sonic_server_response_t *resp);

void sonic_free_server_response(sonic_server_response_t *resp);

sonic_route_t *sonic_add_route(sonic_server_t *server, char *path,
                               sonic_method_t method,
                               sonic_route_func_t route_func);

void sonic_add_middleware(sonic_route_t *route,
                          sonic_middleware_func_t middleware_func,
                          void *user_pointer);

void sonic_add_server_middleware(sonic_server_t *server,
                                 sonic_middleware_func_t middleware_func,
                                 void *user_pointer);

void sonic_add_rate_limiter_middleware(sonic_route_t *route, u64 requests_count,
                                       u64 duration, char *message);

void sonic_add_server_rate_limiter_middleware(sonic_server_t *server,
                                              u64 requests_count, u64 duration,
                                              char *message);

void sonic_add_file_route(sonic_server_t *server, char *path,
                          sonic_method_t method, char *file_path,
                          sonic_content_type_t content_type);

void sonic_add_directory_route(sonic_server_t *server, char *path,
                               char *directory_path);

int sonic_start_server(sonic_server_t *server);

void sonic_stop_server(sonic_server_t *server);

sonic_server_t *sonic_new_server(char *host, u16 port);

sonic_server_response_t *sonic_new_response(sonic_status_t status,
                                            sonic_content_type_t content_type);

sonic_server_response_t *sonic_new_file_response(sonic_status_t status,
                                                 char *file_path);

sonic_middleware_response_t *
sonic_new_middleware_response(bool should_respond,
                              sonic_server_response_t *response);

void sonic_response_set_body(sonic_server_response_t *resp, char *response_body,
                             u64 response_body_size);

void sonic_response_add_header(sonic_server_response_t *resp, char *key,
                               char *value);

char *sonic_get_path_segment(sonic_server_request_t *req, char *segment_name);

sonic_wildcard_segments_t
sonic_get_path_wildcard_segments(sonic_server_request_t *req);

void sonic_free_path_wildcard_segments(sonic_wildcard_segments_t segments);
// ===================================================================================================================

#endif
