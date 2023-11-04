/**** utils.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of the Sonic HTTP library, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SONIC_UTILS_H
#define SONIC_UTILS_H

#include "../sonic.h"

typedef struct {
  char **files;
  u64 files_count;
} sonic_files_list_t;

typedef struct {
  bool failed;
  char *file;
  int line;

  // OK
  void *ok_ptr;
  int ok_int;
  u64 ok_u64;
  bool ok_bool;

  // ERR
  u64 error_code;
  char *error_message;
} result_t;

#define OK(value)                                                              \
  (result_t) {                                                                 \
    .failed = false, .ok_ptr = value, .error_message = NULL,                   \
    .file = strdup(__FILE__), .line = __LINE__                                 \
  }

#define OK_INT(value)                                                          \
  (result_t) {                                                                 \
    .failed = false, .ok_int = value, .error_message = NULL,                   \
    .file = strdup(__FILE__), .line = __LINE__                                 \
  }

#define OK_U64(value)                                                          \
  (result_t) {                                                                 \
    .failed = false, .ok_u64 = value, .error_message = NULL,                   \
    .file = strdup(__FILE__), .line = __LINE__                                 \
  }

#define OK_BOOL(value)                                                         \
  (result_t) {                                                                 \
    .failed = false, .ok_bool = value, .error_message = NULL,                  \
    .file = strdup(__FILE__), .line = __LINE__                                 \
  }

#define ERR(...) __sonic_err(__FILE__, __LINE__, __VA_ARGS__)

#define VALUE(res) res.ok_ptr;

#define VALUE_U64(res) res.ok_u64;

// WARN: DO NOT use a function call as the parameter for PROPAGATE e.g
// PROPAGATE(my_function());
#define PROPAGATE(res)                                                         \
  res.ok_ptr;                                                                  \
  do {                                                                         \
    if (!sonic_is_ok(res)) {                                                   \
      return res;                                                              \
    }                                                                          \
    sonic_free_result(res);                                                    \
  } while (0)

result_t __sonic_err(const char *file, int line, const char *format, ...);
bool sonic_is_ok(result_t res);
bool sonic_is_err(result_t res);
void sonic_free_result(result_t res);

char *utils_status_code_to_string(sonic_status_t status_code);

char *utils_get_header_value(sonic_header_t *headers, u64 headers_count,
                             char *key);

sonic_content_type_t file_extension_to_content_type(char *file_extension);

char *get_file_extension(const char *file_path);

char *remove_prefix(const char *prefix, const char *input);

sonic_files_list_t get_files_list_from_dir(char *dir_path);

void list_files_recursive(sonic_files_list_t *result, char *dir_path);

char *concat_path(char *path1, char *path2);

sonic_method_t str_to_http_method(const char *method_str);

void extract_scheme_from_url(const char *url, char scheme[]);

void extract_hostname_and_port_from_host(const char *host,
                                         sonic_scheme_t scheme, char *hostname,
                                         u16 *port);

void extract_path_from_url(const char *url, char *path);

void extract_host_from_url(const char *url, char *host);

int resolve_domain_to_ip(const char *domain, char *ip_buffer,
                         size_t ip_buffer_size);

bool is_ip_address(const char *str);

char *http_method_to_str(sonic_method_t method);

char *content_type_to_string(sonic_content_type_t content_type);

sonic_status_t number_to_status_code(u16 status);

#endif