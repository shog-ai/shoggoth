/**** netlibc.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of netlibc, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../include/netlibc.h"
#include "../include/netlibc/error.h"
#include "../include/netlibc/fs.h"
#include "../include/netlibc/log.h"
#include "../include/netlibc/string.h"

#include <assert.h>
#include <dirent.h>
#include <stdlib.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/****
 * utility function to check if a directory exists
 *
 ****/
bool dir_exists(char *dir_path) {
#ifdef _WIN32
  DWORD attrib = GetFileAttributesA(dir_path);
  return (attrib != INVALID_FILE_ATTRIBUTES &&
          (attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
  struct stat st;
  if (stat(dir_path, &st) == 0) {
    return S_ISDIR(st.st_mode);
  }
  return false;
#endif
}

/****
 * utility function to check if a file exists
 *
 ****/
bool file_exists(const char *file_path) {
  if (access(file_path, F_OK) != -1) {
    return true; // File exists
  } else {
    return false; // File doesn't exist
  }
}

/****
 * creates a new directory
 *
 ****/
void create_dir(const char *dir_path) {
#ifdef _WIN32
  _mkdir(dir_path);
#else
  mkdir(dir_path, 0777);
#endif
}

/****
 * creates a new file
 *
 ****/
void create_file(const char *file_path) {
  FILE *file = fopen(file_path, "w");
  fclose(file);
}

/****U
 * utility function to extract the file name from a file path
 *
 ****/
char *extract_filename_from_path(char *path) {
  char *filename = strrchr(path, '/');
  if (filename == NULL) {
    // If no '/' is found, check for Windows-style '\' separator
    filename = strrchr(path, '\\');
  }

  if (filename == NULL) {
    // No directory separator found, return the original path
    return path;
  } else {
    // Move the pointer to the next character after the separator
    return filename + 1;
  }
}

/****
 * writes a string to a file
 *
 ****/
result_t write_to_file(const char *file_path, char *content, size_t size) {
  FILE *file = fopen(file_path, "w");
  if (file != NULL) {
    fwrite(content, sizeof(char), size, file);
    fclose(file);
  } else {
    return ERR("Failed to write to file '%s'", file_path);
  }

  return OK(NULL);
}

result_t append_to_file(const char *file_path, char *content, size_t size) {
  FILE *file = fopen(file_path, "a");
  if (file != NULL) {
    fwrite(content, sizeof(char), size, file);
    fclose(file);
  } else {
    return ERR("Failed to append to file '%s'", file_path);
  }

  return OK(NULL);
}

/****U
 * returns the extension of a file path
 * e.g returns `txt` for myfile.txt
 *
 ****/
char *get_file_extension(char *file_path) {
  char *extension = strrchr(file_path, '.');
  if (extension == NULL || extension == file_path) {
    return ""; // No extension found or the dot is the first character
  } else {
    return extension + 1; // Skip the dot and return the extension
  }
}

/****
 * returns the content of a file as an allocated string
 *
 ****/
result_t read_file_to_string(const char *file_path) {
  FILE *file = fopen(file_path, "r");
  if (file == NULL) {
    return ERR("Error opening file: %s : %s", strerror(errno), file_path);
  }

  fseek(file, 0, SEEK_END);
  u32 file_size = (u32)ftell(file);
  fseek(file, 0, SEEK_SET);

  char *file_content = (char *)malloc(file_size + 1);

  size_t bytes_read = fread(file_content, 1, file_size, file);
  if (bytes_read != file_size) {
    return ERR("Error reading file");
  }

  file_content[file_size] = '\0';
  fclose(file);
  return OK(file_content);
}

char *escape_character(char *input, char character) {
  if (input == NULL) {
    return NULL; // Return NULL if the input string is not valid.
  }

  size_t len = strlen(input);
  char *result = (char *)malloc(
      (len * 2 + 1) * sizeof(char)); // Allocate memory for the new string.

  size_t j = 0; // Index for the result string.

  for (size_t i = 0; i < len; i++) {
    if (input[i] == character) {
      result[j++] = '\\';
      result[j++] = character;
    } else {
      result[j++] = input[i];
    }
  }

  result[j] = '\0'; // Null-terminate the result string.

  return result;
}

char *replace_escape_character(char *input, char ch, char replacement) {
  if (input == NULL) {
    return NULL; // Return NULL if the input string is not valid.
  }

  size_t len = strlen(input);
  char *result = (char *)malloc(
      (len * 2 + 1) * sizeof(char)); // Allocate memory for the new string.

  size_t j = 0; // Index for the result string.

  for (size_t i = 0; i < len; i++) {
    if (input[i] == ch) {
      result[j++] = '\\';
      result[j++] = replacement;
    } else {
      result[j++] = input[i];
    }
  }

  result[j] = '\0'; // Null-terminate the result string.

  return result;
}

char *escape_tabs(char *input) {
  if (input == NULL) {
    return NULL; // Return NULL if the input string is not valid.
  }

  size_t len = strlen(input);
  char *result = (char *)malloc(
      (len * 2 + 1) * sizeof(char)); // Allocate memory for the new string.

  size_t j = 0; // Index for the result string.

  for (size_t i = 0; i < len; i++) {
    if (input[i] == '\t') {
      result[j++] = '\\';
      result[j++] = 't';
    } else {
      result[j++] = input[i];
    }
  }

  result[j] = '\0'; // Null-terminate the result string.

  return result;
}

char *escape_json_string(char *i) {
  char *i2 = escape_character(i, '\\');

  char *i3 = escape_character(i2, '"');
  free(i2);

  char *i4 = replace_escape_character(i3, '\n', 'n');
  free(i3);

  char *i5 = replace_escape_character(i4, '\t', 't');
  free(i4);

  char *i6 = replace_escape_character(i5, '\r', 'r');
  free(i5);

  return i6;
}

char *escape_html_tags(char *input) {
  if (input == NULL) {
    return NULL;
  }

  size_t input_len = strlen(input);
  char *result = (char *)malloc(
      (input_len * 6) + 1); // Maximum expansion: 6 times for each character

  size_t j = 0; // Index for the result string

  for (size_t i = 0; i < input_len; i++) {
    if (input[i] == '<') {
      strcpy(&result[j], "&lt;");
      j += 4;
    } else if (input[i] == '>') {
      strcpy(&result[j], "&gt;");
      j += 4;
    } else {
      result[j] = input[i];
      j++;
    }
  }

  result[j] = '\0'; // Null-terminate the result string
  return result;
}

char *remove_newlines_except_quotes(char *input) {
  int input_length = (int)strlen(input);
  char *result = (char *)malloc((unsigned long)input_length + 1);
  if (result == NULL) {
    return NULL; // Memory allocation failed
  }

  int i, j;
  int inside_quotes = 0;

  for (i = 0, j = 0; i < input_length; i++) {
    if (input[i] == '"') {
      inside_quotes = 1 - inside_quotes; // Toggle inside_quotes
      result[j++] = input[i];
    } else if (input[i] != '\n' || inside_quotes) {
      result[j++] = input[i];
    }
  }

  result[j] = '\0'; // Null-terminate the result string

  return result;
}

result_t delete_file(const char *file_path) {
  if (remove(file_path) == 0) {
    return OK(NULL);
  } else {
    return ERR("error deleting file: ", file_path);
  }
}

// Function to recursively delete a directory and its contents
result_t delete_dir(const char *dir_path) {
  struct dirent *entry;
  struct stat statbuf;

  DIR *dir = opendir(dir_path);
  if (dir == NULL) {
    perror("opendir");
    return ERR("could not open dir");
  }

  while ((entry = readdir(dir)) != NULL) {
    char filePath[FILE_PATH_SIZE + 256];
    sprintf(filePath, "%s/%s", dir_path, entry->d_name);

    if (lstat(filePath, &statbuf) == -1) {
      perror("lstat");
      closedir(dir);
      return ERR("lstat error");
    }

    if (S_ISDIR(statbuf.st_mode)) {
      if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
        result_t res = delete_dir(filePath);

        if (!is_ok(res)) {
          closedir(dir);
        }

        PROPAGATE(res);
      }
    } else {
      // Delete regular files
      if (remove(filePath) == -1) {
        perror("remove");
        closedir(dir);
        return ERR("remove failed");
      }
    }
  }

  closedir(dir);

  // Delete the empty directory itself
  if (rmdir(dir_path) == -1) {
    perror("rmdir");
    return ERR("rmdir failed");
  }

  return OK(NULL);
}

// returns the UNIX timestamp in microseconds
u64 get_timestamp_us() {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  return (u64)tv.tv_sec * 1000000L + (unsigned long)tv.tv_usec;
}

// returns the UNIX timestamp in milliseconds
u64 get_timestamp_ms() {
  u64 us = get_timestamp_us();
  return us / 1000; // Convert microseconds to milliseconds
}

// returns the UNIX timestamp in seconds
u64 get_timestamp_s() {
  u64 ms = get_timestamp_ms();
  return ms / 1000; // Convert milliseconds to seconds
}

/****
 * lists all the files in a directory as an array of strings
 *
 * @param dir_path - path of the directory
 *
 * @return a struct containing the array of strings and the count
 ****/
result_t get_files_list(char *dir_path) {
  files_list_t *result = calloc(1, sizeof(files_list_t));
  result->files = NULL;
  result->files_count = 0;

  DIR *dir = opendir(dir_path);
  if (dir == NULL) {
    PANIC("could not open directory:", dir_path);
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG) { // Check if it's a regular file
      result->files_count++;
      result->files =
          (char **)realloc(result->files, result->files_count * sizeof(char *));

      result->files[result->files_count - 1] = strdup(entry->d_name);
    }
  }

  closedir(dir);

  return OK(result);
}

result_t get_files_and_dirs_list(char *dir_path) {
  files_list_t *result = calloc(1, sizeof(files_list_t));
  result->files = NULL;
  result->files_count = 0;

  DIR *dir = opendir(dir_path);
  if (dir == NULL) {
    PANIC("could not open directory:", dir_path);
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
      result->files_count++;
      result->files =
          (char **)realloc(result->files, result->files_count * sizeof(char *));

      result->files[result->files_count - 1] = strdup(entry->d_name);
    }
  }

  closedir(dir);

  return OK(result);
}

void free_files_list(files_list_t *list) {
  for (u64 i = 0; i < list->files_count; i++) {
    free(list->files[i]);
  }

  if (list->files_count > 0) {
    free(list->files);
  }

  free(list);
}

/****
 * extracts only the filename from a file path excluding the extension
 *
 ****/
result_t remove_file_extension(char *str) {
  // Find the last dot (.) in the string
  const char *lastDot = strrchr(str, '.');

  if (lastDot == NULL) {
    // If no dot is found, return a copy of the original string
    return OK(strdup(str));
  } else {
    // Calculate the length of the new string without the extension
    size_t newLength = (size_t)(lastDot - str);

    // Allocate memory for the new string
    char *newStr = (char *)malloc(newLength + 1);

    if (newStr != NULL) {
      // Copy the characters from the original string to the new string
      strncpy(newStr, str, newLength);
      newStr[newLength] = '\0'; // Null-terminate the new string
    }

    return OK(newStr);
  }
}

/****
 * duplicates a file
 *
 ****/
result_t copy_file(const char *source_file_path,
                   const char *destination_file_path) {
  FILE *sourceFile = fopen(source_file_path, "rb");
  if (sourceFile == NULL) {
    return ERR("Failed to open source file");
  }

  FILE *destinationFile = fopen(destination_file_path, "wb");
  if (destinationFile == NULL) {
    return ERR("Failed to open destination file");
  }

  int ch;
  while ((ch = fgetc(sourceFile)) != EOF) {
    fputc(ch, destinationFile);
  }

  fclose(sourceFile);
  fclose(destinationFile);

  return OK(NULL);
}

// Function to recursively copy a directory and its contents
result_t copy_dir(char *src_path, char *dest_path) {
  struct dirent *entry;
  struct stat statbuf;

  DIR *srcDir = opendir(src_path);
  if (srcDir == NULL) {
    perror("opendir");
    return ERR("failed to open dir");
  }

  if (mkdir(dest_path, 0777) == -1) {
    if (errno != EEXIST) {
      perror("mkdir");
      return ERR("mkdir failed");
    }
  }

  while ((entry = readdir(srcDir)) != NULL) {
    char srcFile[FILE_PATH_SIZE + 256];
    char destFile[FILE_PATH_SIZE + 256];
    sprintf(srcFile, "%s/%s", src_path, entry->d_name);
    sprintf(destFile, "%s/%s", dest_path, entry->d_name);

    if (lstat(srcFile, &statbuf) == -1) {
      perror("lstat");
      closedir(srcDir);
      return ERR("lstat error");
    }

    if (S_ISDIR(statbuf.st_mode)) {
      if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
        result_t res = copy_dir(srcFile, destFile);
        PROPAGATE(res);
      }
    } else {
      // Copy regular files
      FILE *srcFilePtr = fopen(srcFile, "rb");
      FILE *destFilePtr = fopen(destFile, "wb");

      if (srcFilePtr == NULL || destFilePtr == NULL) {
        perror("fopen");
        closedir(srcDir);
        return ERR("open error");
      }

      int ch;
      while ((ch = fgetc(srcFilePtr)) != EOF) {
        fputc(ch, destFilePtr);
      }

      fclose(srcFilePtr);
      fclose(destFilePtr);
    }
  }

  closedir(srcDir);
  return OK(NULL);
}

result_t get_file_size(const char *filePath) {
  FILE *file = fopen(filePath, "rb"); // Open the file in binary mode

  if (file == NULL) {
    perror("Error opening the file");
    return ERR("could not open file");
  }

  fseek(file, 0, SEEK_END); // Move the file pointer to the end of the file
  u64 fileSize =
      (u64)ftell(file); // Get the current position, which is the file size

  fclose(file); // Close the file

  return OK_U64(fileSize);
}

result_t acquire_file_lock(char *file_path, u64 delay, u64 timeout) {
  char lockfile_path[256];
  sprintf(lockfile_path, "%s.lockfile", file_path);

  u64 start_time = get_timestamp_ms();

  int lockfile_fd = -1;

  for (;;) {
    u64 now = get_timestamp_ms();

    if (now > (start_time + timeout)) {
      LOG(INFO, "acquire lock timeout on %s", file_path);
      return ERR("acquire lock timeout on %s \n", file_path);
    }

    // Try to create the lock file exclusively, failing if it already exists
    lockfile_fd = open(lockfile_path, O_CREAT | O_EXCL, 0666);

    if (lockfile_fd == -1) {
      if (errno == EEXIST) {
        LOG(INFO, "waiting for file lock on %s", file_path);
      } else {
        perror("Lockfile creation failed");
        return ERR("Lockfile creation failed");
      }

      usleep((unsigned int)(delay * 1000));
    } else {
      // LOG(INFO, "acquired file lock on %s", file_path);
      break;
    }
  }

  file_lock_t *lock = calloc(1, sizeof(file_lock_t));
  lock->fd = lockfile_fd;
  lock->path = strdup(file_path);

  return OK(lock);
}

void release_file_lock(file_lock_t *lock) {
  char lockfile_path[256];
  sprintf(lockfile_path, "%s.lockfile", lock->path);

  remove(lockfile_path);

  // LOG(INFO, "released file lock on %s", lock->path);

  close(lock->fd);
  free(lock->path);

  free(lock);
}

result_t is_file_plain_text(const char *file_path) {
  FILE *file = fopen(file_path, "rb");
  if (file == NULL) {
    // Unable to open the file
    return ERR("unable to open file");
  }

  int c;
  while ((c = fgetc(file)) != EOF) {
    // Check if the character is a control character (non-printable)
    if ((c >= 0 && c <= 8) || (c >= 14 && c <= 31) || c == 127) {
      fclose(file);
      return OK_BOOL(
          false); // Found a non-printable character, so it's not plain text
    }
  }

  fclose(file);
  return OK_BOOL(true); // No non-printable characters found, it's plain text
}

u64 random_number(u64 start, u64 end) {
  if (start >= end) {
    // Invalid input, return 0 or handle the error as needed.
    return 0;
  }

  // Initialize the random number generator with the current time.
  srand((u32)time(NULL));

  // Generate a random number within the specified range.
  u64 random_num = (u64)start + (u64)rand() % (u64)(end - start + 1);

  return random_num;
}

result_t map_file(char *file_path) {
  file_mapping_t *file_mapping = calloc(1, sizeof(file_mapping_t));

  file_mapping->fd = open(file_path, O_RDONLY);
  if (file_mapping->fd == -1) {
    perror("Error opening file");
    PANIC("Error opening file");
  }

  if (fstat(file_mapping->fd, &file_mapping->info) == -1) {
    perror("Error getting file information");
    close(file_mapping->fd);
    PANIC("Error getting file information");
  }

  // Map the file into memory
  file_mapping->content = mmap(0, (size_t)file_mapping->info.st_size, PROT_READ,
                               MAP_PRIVATE, file_mapping->fd, 0);
  if (file_mapping->content == MAP_FAILED) {
    perror("Error mapping file to memory");
    close(file_mapping->fd);
    exit(1);
  }

  return OK(file_mapping);
}

void unmap_file(file_mapping_t *file_mapping) {
  munmap(file_mapping->content, (size_t)file_mapping->info.st_size);

  close(file_mapping->fd);

  free(file_mapping);
}

result_t get_dir_size(char *path) {
  struct stat statbuf;
  u64 total_size = 0;

  if (stat(path, &statbuf) == -1) {
    // Error handling if stat() fails
    perror("Failed to get directory information");
    return ERR("could not get dir info");
  }

  if (S_ISDIR(statbuf.st_mode)) {
    DIR *dir = opendir(path);
    struct dirent *entry;

    if (dir == NULL) {
      // Error handling if opendir() fails
      perror("Failed to open directory");
      return ERR("could not open dir");
    }

    while ((entry = readdir(dir))) {
      if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
        char child_path[512]; // Adjust the size as needed
        snprintf(child_path, sizeof(child_path), "%s/%s", path, entry->d_name);

        result_t res_child_size = get_dir_size(child_path);
        u64 child_size = PROPAGATE_U64(res_child_size);

        total_size += child_size;
      }
    }

    closedir(dir);
  } else {
    total_size = (u64)statbuf.st_size;
  }

  return OK_U64(total_size);
}

#define RED_COLOR 227, 61, 45
#define YELLOW_COLOR 227, 224, 45

void set_color_rgb(u8 r, u8 g, u8 b) { printf("\e[38;2;%d;%d;%dm", r, g, b); }

void reset_color() { printf("\e[0m"); }

void modify_symbol_string(char *input) {
  char *openParen = strchr(input, '(');
  if (openParen != NULL) {
    *openParen = ' ';
    char *closeParen = strchr(openParen, ')');
    if (closeParen != NULL) {
      *(closeParen) = '\0';
    }
  }
}

void print_backtrace() {
  void *call_stack[10];
  int num_frames = backtrace(call_stack, 10);
  char **symbols = backtrace_symbols(call_stack, num_frames);

  if (symbols == NULL) {
    fprintf(stdout, "Failed to retrieve the backtrace symbols.\n");
    return;
  }

  fprintf(stdout, "Backtrace:\n");
  for (int i = 1; i < num_frames; i++) {
    char *current_symbol = strdup(symbols[i]);
    modify_symbol_string(current_symbol);

    fprintf(stdout, "[%d] %s\n", i, symbols[i]);

    char addr2line_command[256];
    snprintf(addr2line_command, sizeof(addr2line_command),
             "addr2line -e %s -f -p -C", current_symbol);

    FILE *addr2line_output = popen(addr2line_command, "r");
    if (addr2line_output) {
      char buffer[256];
      while (fgets(buffer, sizeof(buffer), addr2line_output) != NULL) {
        fprintf(stdout, "  %s", buffer);
      }
      pclose(addr2line_output);
    }

    free(current_symbol);
  }

  free(symbols);
}

/****
 * custom panic function. used for unrecoverable errors. Prints an error
 *message and exits the process immediatelly. This is an internal funtion
 *exposed by the PANIC macro defined in utils.h
 *
 ****/
void __netlibc_panic(const char *file, int line, const char *format, ...) {
  va_list args;
  va_start(args, format);

  set_color_rgb(RED_COLOR);
  fprintf(stdout, "[PANIC] ");
  reset_color();

  fprintf(stdout, "at %s:%d: ", file, line);

  vfprintf(stdout, format, args);
  fprintf(stdout, "\n");

  print_backtrace();

  fflush(stdout);

  va_end(args);

  exit(1);
}

void __netlibc_exit(const char *file, int line, int status, const char *format,
                    ...) {
  file = (char *)file;
  line = (int)line;

  va_list args;
  va_start(args, format);

#ifndef NDEBUG
  fprintf(stdout, "EXIT with status %d at %s:%d:\n", status, file, line);
#endif

  set_color_rgb(RED_COLOR);
  fprintf(stdout, "Error: ");
  reset_color();

  vfprintf(stdout, format, args);
  fprintf(stdout, "\n");

  fflush(stdout);

  va_end(args);

  exit(status);
}

/****
 * custom log function. sometimes used for print debbuging.
 * This is an internal funtion exposed by the LOG macro defined in utils.h
 *
 ****/
void __netlibc_log(log_level_t log_level, const char *file, int line,
                   const char *format, ...) {
  va_list args;
  va_start(args, format);

  // fprintf(stdout, "LOG at %s:%d: ", file, line);
  if (log_level == INFO) {
    fprintf(stdout, "[INFO]: ");
  } else if (log_level == WARN) {
    set_color_rgb(YELLOW_COLOR);
    fprintf(stdout, "[WARN]: ");
    reset_color();
  } else if (log_level == ERROR) {
    set_color_rgb(RED_COLOR);
    fprintf(stdout, "[ERROR] at %s:%d: ", file, line);
    reset_color();
  }

  vfprintf(stdout, format, args);

  printf("\n");

  fflush(stdout);

  va_end(args);
}

result_t __netlibc_err(const char *file, int line, const char *format, ...) {
  char *error_message = NULL;

  va_list args;
  va_start(args, format);

  char buf[512];
  vsprintf(buf, format, args);

  if (error_message == NULL) {
    error_message = malloc((strlen(buf) + 1) * sizeof(char));
    strcpy(error_message, buf);
  } else {
    error_message =
        realloc(error_message,
                (strlen(error_message) + strlen(buf) + 1) * sizeof(char));
    strcat(error_message, buf);
  }

  va_end(args);

  assert(error_message != NULL);

  return (result_t){.failed = true,
                    .ok_ptr = NULL,
                    .error_message = error_message,
                    .file = strdup(file),
                    .line = line};
}

bool is_ok(result_t res) {
  if (res.failed) {
    return false;
  } else {
    return true;
  }
}

void free_result(result_t res) {
  if (!is_ok(res)) {
    free(res.error_message);
  }

  free(res.file);
}

void *__unwrap(const char *file, int line, result_t res) {
  void *res_value = NULL;

  if (!is_ok(res)) {
    char *panic_msg =
        malloc((strlen(res.error_message) + 100 + 1) * sizeof(char));
    sprintf(panic_msg, "unwrap of error value from %s:%d -> %s", res.file,
            res.line, res.error_message);

    __netlibc_panic(file, line, panic_msg);
  } else {
    res_value = res.ok_ptr;
  }

  free_result(res);

  return res_value;
}

int __unwrap_int(const char *file, int line, result_t res) {
  int res_value = 0;

  if (!is_ok(res)) {
    char *panic_msg =
        malloc((strlen(res.error_message) + 100 + 1) * sizeof(char));
    sprintf(panic_msg, "unwrap of error value from %s:%d -> %s", res.file,
            res.line, res.error_message);

    __netlibc_panic(file, line, panic_msg);
  } else {
    res_value = res.ok_int;
  }

  free_result(res);

  return res_value;
}

u64 __unwrap_u64(const char *file, int line, result_t res) {
  u64 res_value = 0;

  if (!is_ok(res)) {
    char *panic_msg =
        malloc((strlen(res.error_message) + 100 + 1) * sizeof(char));
    sprintf(panic_msg, "unwrap of error value from %s:%d -> %s", res.file,
            res.line, res.error_message);

    __netlibc_panic(file, line, panic_msg);
  } else {
    res_value = res.ok_u64;
  }

  free_result(res);

  return res_value;
}

bool __unwrap_bool(const char *file, int line, result_t res) {
  bool res_value = false;

  if (!is_ok(res)) {
    char *panic_msg =
        malloc((strlen(res.error_message) + 100 + 1) * sizeof(char));
    sprintf(panic_msg, "unwrap of error value from %s:%d -> %s", res.file,
            res.line, res.error_message);

    __netlibc_panic(file, line, panic_msg);
  } else {
    res_value = res.ok_bool;
  }

  free_result(res);

  return res_value;
}

bool is_err(result_t res) {
  if (!res.failed) {
    return false;
  } else {
    return true;
  }
}
