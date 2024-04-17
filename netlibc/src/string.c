/**** string.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of netlibc, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/netlibc/mem.h"

char *string_from(const char *str1, ...) {
  va_list args;
  va_start(args, str1);

  // Calculate the total length of the concatenated string
  size_t total_length = strlen(str1);

  // Concatenate the strings
  const char *current_str = va_arg(args, const char *);
  while (current_str != NULL) {
    total_length += strlen(current_str);
    current_str = va_arg(args, const char *);
  }

  va_end(args);

  // Allocate memory for the concatenated string
  char *result = (char *)nmalloc(total_length + 1);

  if (result == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }

  // Concatenate the strings again
  strcpy(result, str1);

  va_start(args, str1);
  current_str = va_arg(args, const char *);
  while (current_str != NULL) {
    strcat(result, current_str);
    current_str = va_arg(args, const char *);
  }

  va_end(args);

  return result;
}

// creatse a copy of a string and frees the original copy, but uses the netlibc
// allocator
char *nstringify(char *str) {
  char *new_str = nmalloc((strlen(str) + 1) * sizeof(char));
  strcpy(new_str, str);

  free(str);

  return new_str;
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

void print_string_as_ascii(char *input) {
  if (input == NULL) {
    return; // Handle null input.
  }

  // Loop through each character of the string until the null terminator.
  for (int i = 0; input[i] != '\0'; i++) {
    printf("%d ",
           (unsigned char)input[i]); // Print ASCII value of each character.
  }
  printf("\n"); // Print a new line at the end.
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
