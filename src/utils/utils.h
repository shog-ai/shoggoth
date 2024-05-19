/**** utils.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef UTILS_H
#define UTILS_H

#include <netlibc.h>
#include <netlibc/error.h>
#include <netlibc/fs.h>
#include <netlibc/log.h>
#include <netlibc/string.h>

#include <sys/stat.h>

// #define FILE_PATH_SIZE 256

#define SHOGGOTH_ID_SIZE 512
#define NODE_ID_SIZE 512

struct NODE_CTX;
struct CLIENT_CTX;
struct STUDIO_CTX;

char *utils_gen_uuid();

result_t utils_clear_dir_timestamps(char *dir_path);

result_t utils_clear_dir_permissions(char *dir_path);

result_t utils_create_tarball(char *dir_path, char *output_path);

result_t utils_hash_tarball(char *tmp_path, char *tarball_path);

result_t utils_extract_tarball(char *archive_path, char *destination_path);

char *utils_strip_public_key(const char *input);

bool utils_keys_exist(char *keys_path);

void utils_get_node_runtime_path(struct NODE_CTX *ctx,
                                 char node_runtime_path[]);

void utils_get_client_runtime_path(struct CLIENT_CTX *ctx,
                                   char client_runtime_path[]);

void utils_get_node_explorer_path(struct NODE_CTX *ctx,
                                  char node_explorer_path[]);

void utils_get_node_tmp_path(struct NODE_CTX *ctx, char node_tmp_path[]);

void utils_get_client_tmp_path(struct CLIENT_CTX *ctx, char client_tmp_path[]);

void utils_get_node_update_path(struct NODE_CTX *ctx, char node_update_path[]);

int utils_get_default_runtime_path(char runtime_path[]);

char *utils_get_logs_path(char *home_path);

void utils_verify_node_runtime_dirs(struct NODE_CTX *ctx);

void utils_verify_studio_runtime_dirs(struct STUDIO_CTX *ctx);

void utils_verify_client_runtime_dirs(struct CLIENT_CTX *ctx);

#endif
