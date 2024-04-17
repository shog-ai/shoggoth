#ifndef NETLIBC_FS_H
#define NETLIBC_FS_H

// #define FILE_PATH_SIZE 256

#include "../netlibc.h"
#include "./error.h"

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

result_t acquire_file_lock(char *file_path, u64 delay, u64 timeout);
void release_file_lock(file_lock_t *lock);

result_t is_file_plain_text(const char *file_path);

result_t map_file(char *file_path);
void unmap_file(file_mapping_t *file_mapping);

result_t get_dir_size(char *path);

result_t get_file_size(const char *filePath);

result_t clear_dir_timestamps(char *dir_path);

result_t clear_dir_permissions(char *dir_path);

result_t delete_file(const char *file_path);

result_t delete_dir(const char *dir_path);

result_t copy_dir(char *src_path, char *dest_path);

result_t get_files_list(char *dir_path);

result_t get_files_and_dirs_list(char *dir_path);

void free_files_list(files_list_t *list);

result_t copy_file(const char *source_file_path,
                         const char *destination_file_path);

void create_dir(const char *dir_path);

void create_file(const char *file_path);

bool dir_exists(char *dir_path);

bool file_exists(const char *file_path);

result_t write_to_file(const char *file_path, char *content, size_t size);

result_t append_to_file(const char *file_path, char *content,
                              size_t size);

result_t read_file_to_string(const char *file_path);

#endif
