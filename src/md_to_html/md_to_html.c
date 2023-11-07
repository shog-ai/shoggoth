#include "../../lib/md4c/src/md4c-html.h"

#include "../utils/utils.h"
#include "md_to_html.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *data;
  int size;
} my_data_t;

void my_callback(const MD_CHAR *data, MD_SIZE data_size, void *user_data) {
  my_data_t *casted_data = (my_data_t *)user_data;

  if (casted_data->size == 0) {
    casted_data->data = malloc((data_size + 1) * sizeof(char));

    strncpy(casted_data->data, data, data_size);
    casted_data->data[data_size] = '\0';
  } else {
    casted_data->data = realloc(
        casted_data->data, (casted_data->size + data_size + 1) * sizeof(char));

    strncat(casted_data->data, data, data_size);

    casted_data->data[casted_data->size + data_size] = '\0';
  }

  casted_data->size += data_size;
}

result_t generate_html(char *input) {
  my_data_t *my_data = calloc(1, sizeof(my_data_t));
  my_data->size = 0;
  my_data->data = NULL;

  int error = md_html(input, strlen(input), my_callback, my_data, 0, 0);
  assert(error == 0);

  char *output_str = my_data->data;
  free(my_data);

  return OK(output_str);
}

result_t merge_output(char *content, head_type_t head_type,
                      end_type_t end_type) {
  char *head = NULL;
  char *end = NULL;

  if (head_type == HEAD_DOCS) {
    result_t res_head = read_file_to_string("./docs/templates/head.html");
    head = PROPAGATE(res_head);
  } else if (head_type == HEAD_API) {
    result_t res_head =
        read_file_to_string("./docs/templates/head-api.html");
    head = PROPAGATE(res_head);
  }

  if (end_type == END_DOCS) {
    result_t res_end = read_file_to_string("./docs/templates/end.html");
    end = PROPAGATE(res_end);
  }

  char *final_output =
      malloc((strlen(head) + strlen(content) + strlen(end) + 1) * sizeof(char));

  strcpy(final_output, head);
  strcat(final_output, content);
  strcat(final_output, end);

  return OK(final_output);
}

result_t md_to_html(char *md_str) {
  result_t res_html = generate_html(md_str);

  return res_html;
}

result_t md_file_to_html_file(char *input_path, char *output_path) {
  result_t res_input = read_file_to_string(input_path);
  char *input = PROPAGATE(res_input);

  assert(input != NULL);

  result_t res_output = md_to_html(input);

  char *output = PROPAGATE(res_output);

  write_to_file(output_path, output, strlen(output));

  free(output);
  free(input);

  return OK(NULL);
}
