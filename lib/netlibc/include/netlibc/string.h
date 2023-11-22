#ifndef NETLIBC_STRING_H
#define NETLIBC_STRING_H

#include "../netlibc.h"
#include "./error.h"

// typedef struct {
// char *str;
// } string_t;

char *new_string();

char *string_from(const char *str1, ...);

void free_string(char *str);

char *remove_newlines_except_quotes(char *input);

char *escape_newlines(char *input);

char *escape_json_string(char *i);

char *escape_html_tags(char *input);

result_t remove_file_extension(char *str);

char *extract_filename_from_path(char *path);

char *get_file_extension(char *file_path);

#endif