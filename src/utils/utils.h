/**** utils.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef UTILS_H
#define UTILS_H

#include <netlibc.h>
#include <sys/stat.h>
#include <netlibc.h>

#define FILE_PATH_SIZE 256

#define SHOGGOTH_ID_SIZE 512
#define NODE_ID_SIZE 512

struct NODE_CTX;
struct CLIENT_CTX;

typedef struct {
  int fd;
  char *path;
} file_lock_t;

typedef struct {
  char *content;
  int fd;
  struct stat info;
} file_mapping_t;

typedef struct {
  char **files;
  u64 files_count;
} files_list_t;

#define SERVER_ERR(res)                                                        \
  do {                                                                         \
    if (is_err(res)) {                                                         \
      respond_error(req, res.error_message);                                   \
      free_result(res);                                                        \
      return;                                                                  \
    }                                                                          \
                                                                               \
    free_result(res);                                                          \
  } while (0)

result_t utils_acquire_file_lock(char *file_path, u64 delay, u64 timeout);
void utils_release_file_lock(file_lock_t *lock);


char *utils_gen_uuid();

result_t utils_is_file_plain_text(const char *file_path);

u64 utils_random_number(u64 start, u64 end);

result_t utils_map_file(char *file_path);
void utils_unmap_file(file_mapping_t *file_mapping);

result_t utils_get_dir_size(char *path);

result_t utils_get_file_size(const char *filePath);

char *utils_remove_newlines_except_quotes(const char *input);

char *utils_escape_newlines(char *input);

char *utils_escape_json_string(char *i);

char *utils_escape_html_tags(const char *input);

result_t utils_clear_dir_timestamps(char *dir_path);

result_t utils_clear_dir_permissions(char *dir_path);

result_t utils_create_tarball(char *dir_path, char *output_path);

result_t utils_hash_tarball(char *tmp_path, char *tarball_path);

result_t utils_extract_tarball(char *archive_path, char *destination_path);

result_t utils_delete_file(const char *file_path);

result_t utils_delete_dir(const char *dir_path);

result_t utils_copy_dir(char *src_path, char *dest_path);

u64 utils_get_timestamp_us();

u64 utils_get_timestamp_ms();

u64 utils_get_timestamp_s();

result_t utils_get_files_list(char *dir_path);

result_t utils_get_files_and_dirs_list(char *dir_path);

void utils_free_files_list(files_list_t *list);

result_t utils_remove_file_extension(const char *str);

result_t utils_copy_file(const char *source_file_path,
                         const char *destination_file_path);

char *utils_strip_public_key(const char *input);

bool utils_keys_exist(char *keys_path);

void utils_get_node_runtime_path(struct NODE_CTX *ctx,
                                 char node_runtime_path[]);

void utils_get_client_runtime_path(struct CLIENT_CTX *ctx,
                                   char client_runtime_path[]);

void utils_get_node_explorer_path(struct NODE_CTX *ctx,
                                  char node_explorer_path[]);

void utils_get_node_bin_path(struct NODE_CTX *ctx, char node_bin_path[]);

void utils_get_node_tmp_path(struct NODE_CTX *ctx, char node_tmp_path[]);

void utils_get_client_tmp_path(struct CLIENT_CTX *ctx, char client_tmp_path[]);

void utils_get_node_update_path(struct NODE_CTX *ctx, char node_update_path[]);

int utils_get_default_runtime_path(char runtime_path[]);

char *utils_get_logs_path(char *home_path);

const char *utils_extract_filename_from_path(const char *path);

const char *utils_get_file_extension(const char *file_path);

void utils_create_dir(const char *dir_path);

void utils_create_file(const char *file_path);

bool utils_dir_exists(char *dir_path);

bool utils_file_exists(const char *file_path);

void utils_verify_node_runtime_dirs(struct NODE_CTX *ctx);

void utils_verify_client_runtime_dirs(struct CLIENT_CTX *ctx);

result_t utils_write_to_file(const char *file_path, char *content, size_t size);

result_t utils_append_to_file(const char *file_path, char *content,
                              size_t size);

result_t utils_read_file_to_string(const char *file_path);

#endif