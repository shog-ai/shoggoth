/**** utils.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Sonic HTTP library, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SONIC_UTILS_H
#define SONIC_UTILS_H

#include <netlibc.h>
#include <netlibc/error.h>
#include <netlibc/fs.h>
#include <netlibc/log.h>
#include <netlibc/string.h>

#include "../sonic.h"

result_t utils_status_code_to_string(sonic_status_t status_code);

char *utils_get_header_value(sonic_header_t *headers, u64 headers_count,
                             char *key);

sonic_content_type_t file_extension_to_content_type(char *file_extension);

// char *get_file_extension(const char *file_path);

char *remove_prefix(const char *prefix, const char *input);

files_list_t *get_files_list_from_dir(char *dir_path);

void list_files_recursive(files_list_t *result, char *dir_path);

char *concat_path(char *path1, char *path2);

result_t str_to_http_method(const char *method_str);

void extract_scheme_from_url(const char *url, char scheme[]);

void extract_hostname_and_port_from_host(const char *host,
                                         sonic_scheme_t scheme, char *hostname,
                                         u16 *port);

void extract_path_from_url(const char *url, char *path);

void extract_host_from_url(const char *url, char *host);

int resolve_domain_to_ip(const char *domain, char *ip_buffer,
                         size_t ip_buffer_size);

bool is_ip_address(const char *str);

result_t http_method_to_str(sonic_method_t method);

result_t content_type_to_string(sonic_content_type_t content_type);

result_t number_to_status_code(u16 status);

#endif
