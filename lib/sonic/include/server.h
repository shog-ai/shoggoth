/**** server.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Sonic HTTP library, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SONIC_SERVER_H
#define SONIC_SERVER_H

#include "../sonic.h"

sonic_wildcard_segments_t
server_get_path_wildcard_segments(sonic_server_request_t *req);

void server_free_path_wildcard_segments(sonic_wildcard_segments_t segments);

char *server_get_path_segment(sonic_server_request_t *req, char *segment_name);

void server_send_response(sonic_server_request_t *req,
                          sonic_server_response_t *resp);

void free_server_response(sonic_server_response_t *resp);

sonic_route_t *server_add_route(sonic_server_t *server, char *path,
                                sonic_method_t method,
                                sonic_route_func_t route_func);

void server_add_route_middleware(sonic_route_t *route,
                                 sonic_middleware_func_t middleware_func,
                                 void *user_pointer);

void server_add_server_middleware(sonic_server_t *server,
                                  sonic_middleware_func_t middleware_func,
                                  void *user_pointer);

void server_add_rate_limiter_middleware(sonic_route_t *route,
                                        u64 requests_count, u64 duration,
                                        char *message);

void server_add_server_rate_limiter_middleware(sonic_server_t *server,
                                               u64 requests_count, u64 duration,
                                               char *message);

sonic_middleware_response_t *
new_server_middleware_response(bool should_respond,
                               sonic_server_response_t *response);

void server_add_file_route(sonic_server_t *server, char *path,
                           sonic_method_t method, char *file_path,
                           sonic_content_type_t content_type);

void server_add_directory_route(sonic_server_t *server, char *path,
                                char *directory_path);

int start_server(sonic_server_t *server);

void stop_server(sonic_server_t *server);

sonic_server_t *new_server(char *host, u16 port);

sonic_server_response_t *new_server_response(sonic_status_t status,
                                             sonic_content_type_t content_type);

sonic_server_response_t *new_server_file_response(sonic_status_t status,
                                                  char *file_path);

void server_response_set_body(sonic_server_response_t *resp,
                              char *response_body, u64 response_body_size);

void server_response_add_header(sonic_server_response_t *resp, char *key,
                                char *value);

#endif
