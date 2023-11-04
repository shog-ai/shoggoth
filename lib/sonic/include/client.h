/**** client.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of the Sonic HTTP library, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "../sonic.h"
#include "utils.h"

void client_set_redirect_policy(sonic_client_request_t *req,
                                sonic_redirect_policy_t policy);

void client_set_response_callback(sonic_client_request_t *req,
                                  response_callback_func_t callback_func,
                                  void *user_pointer);

void client_request_add_header(sonic_client_request_t *req, char *key,
                               char *value);

sonic_client_response_t *fail_response(char *error);

void http_parse_response_head(sonic_client_response_t *resp, char *head_buffer);

int http_extract_status_code(const char *header, u16 *status_code);

sonic_client_response_t *new_client_response();

sonic_client_request_t *new_client_request(sonic_method_t method, char *url);

void client_free_request(sonic_client_request_t *req);

void client_free_response(sonic_client_response_t *req);

sonic_client_response_t *client_send_request(sonic_client_request_t *req);

void client_request_set_body(sonic_client_request_t *req, char *request_body,
                             u64 request_body_size);

#endif