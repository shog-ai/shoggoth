/**** string.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of netlibc, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include <stdlib.h>
#include <string.h>

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
