/**** templating.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

// #include "../utils/utils.h"

#include "../handlebazz.h"
#include "../../src/include/cjson.h"

#include <netlibc/log.h>

#include <assert.h>
#include <stdlib.h>

typedef enum { VALUE, FOR_LOOP, IF_CONDITION, PARTIAL } command_type_t;

typedef struct {
  command_type_t command_type;
  char *raw_command;
  cJSON *data;
  char *key;

  // FOR LOOP
  char *loop_variable;
  char *loop_body;

  // IF CONDITION
  char *condition_variable;
  char *condition_body;
} template_command_t;

template_t *create_template(char *template_string, char *template_data) {
  template_t *template_object = calloc(1, sizeof(template_t));

  template_object->template_string = strdup(template_string);
  template_object->template_data = strdup(template_data);

  template_object->partials = NULL;
  template_object->partials_count = 0;

  return template_object;
}

result_t template_add_partial(template_t *parent, char *partial_name,
                              template_t *partial_template) {

  parent->partials = realloc(parent->partials, (parent->partials_count + 1) *
                                                   sizeof(template_partial_t));

  parent->partials[parent->partials_count].partial_template = partial_template;
  parent->partials[parent->partials_count].partial_name = strdup(partial_name);

  parent->partials_count++;

  return OK(NULL);
}

void free_template(template_t *template_object) {
  free(template_object->template_string);
  free(template_object->template_data);

  if (template_object->partials_count > 0) {
    for (u64 i = 0; i < template_object->partials_count; i++) {
      free(template_object->partials[i].partial_name);
    }

    free(template_object->partials);
  }

  free(template_object);
}

result_t add_to_buffer(char **buffer, u64 *buffer_size, char ch) {
  char *new_buffer = realloc(*buffer, (*buffer_size + 1) * sizeof(char));
  new_buffer[*buffer_size] = ch;

  *buffer = new_buffer;

  *buffer_size = *buffer_size + 1;

  return OK(NULL);
}

result_t get_json_object(char *key, cJSON *json) {
  cJSON *value_json = cJSON_GetObjectItemCaseSensitive(json, key);

  if (value_json == NULL) {
    return ERR("could not get object `%s` from json", key);
  }

  return OK(value_json);
}

result_t get_json_string(char *key, cJSON *json) {
  result_t res_value_json = get_json_object(key, json);
  cJSON *value_json = PROPAGATE(res_value_json);

  if (!cJSON_IsString(value_json)) {
    return ERR("value type is not string");
  }

  char *data_value = value_json->valuestring;

  return OK(data_value);
}

result_t get_json_bool(char *key, cJSON *json) {
  cJSON *value_json = cJSON_GetObjectItemCaseSensitive(json, key);

  if (value_json == NULL) {
    return OK_BOOL(false);
  }

  if (!cJSON_IsBool(value_json)) {
    return ERR("value type is not bool");
  }

  if (cJSON_IsTrue(value_json)) {
    return OK_BOOL(true);
  } else {
    return OK_BOOL(false);
  }
}

char *get_json_anytype(cJSON *json) {
  char *value_str = NULL;

  if (cJSON_IsString(json)) {
    value_str = strdup(json->valuestring);
  } else {
    value_str = cJSON_Print(json);
  }

  return value_str;
}

result_t get_json_array(char *key, cJSON *json) {
  result_t res_value_json = get_json_object(key, json);
  cJSON *value_json = PROPAGATE(res_value_json);

  if (!cJSON_IsArray(value_json)) {
    return ERR("value is not an array type");
  }

  return OK(value_json);
}

result_t process_command(template_t *template_object, char **buffer,
                         u64 *buffer_size, template_command_t com) {
  if (com.command_type == VALUE) {
    if (strcmp(com.key, "this") == 0) {
      char *value = get_json_anytype(com.data);

      for (u64 i = 0; i < strlen(value); i++) {
        result_t res_add = add_to_buffer(buffer, buffer_size, value[i]);
        PROPAGATE(res_add);
      }

      free(value);
    } else if (strlen(com.key) > 5 && strncmp(com.key, "this.", 5) == 0) {
      result_t res_value_json = get_json_object(&com.key[5], com.data);
      cJSON *value_json = PROPAGATE(res_value_json);

      char *value = get_json_anytype(value_json);

      for (u64 i = 0; i < strlen(value); i++) {
        result_t res_add = add_to_buffer(buffer, buffer_size, value[i]);
        PROPAGATE(res_add);
      }
    } else {
      result_t res_value_json = get_json_object(com.key, com.data);
      cJSON *value_json = PROPAGATE(res_value_json);

      char *value = get_json_anytype(value_json);

      for (u64 i = 0; i < strlen(value); i++) {
        result_t res_add = add_to_buffer(buffer, buffer_size, value[i]);
        PROPAGATE(res_add);
      }

      free(value);
    }
  } else if (com.command_type == FOR_LOOP) {
    cJSON *value = NULL;

    if (strlen(com.loop_variable) > 5 &&
        strncmp(com.loop_variable, "this.", 5) == 0) {
      result_t res_value = get_json_array(&com.loop_variable[5], com.data);
      value = PROPAGATE(res_value);
    } else {
      result_t res_value = get_json_array(com.loop_variable, com.data);
      value = PROPAGATE(res_value);
    }

    u64 array_size = (u64)cJSON_GetArraySize(value);

    for (u64 i = 0; i < array_size; i++) {
      char *iteration_value = com.loop_body;

      cJSON *iteration_data_json = cJSON_GetArrayItem(value, (int)i);

      result_t res_cooked_iteration_value = cook_block_template(
          template_object, iteration_value, com.data, iteration_data_json, 1);
      char *cooked_iteration_value = PROPAGATE(res_cooked_iteration_value);

      for (u64 k = 0; k < strlen(cooked_iteration_value); k++) {
        result_t res_add =
            add_to_buffer(buffer, buffer_size, cooked_iteration_value[k]);
        PROPAGATE(res_add);
      }

      free(cooked_iteration_value);
    }
  } else if (com.command_type == IF_CONDITION) {
    bool bool_value = false;

    if (strlen(com.condition_variable) > 5 &&
        strncmp(com.condition_variable, "this.", 5) == 0) {

      if (com.condition_variable[5] == '!') {
        result_t res_bool_value =
            get_json_bool(&com.condition_variable[6], com.data);
        bool_value = !PROPAGATE_BOOL(res_bool_value);
      } else {
        result_t res_bool_value =
            get_json_bool(&com.condition_variable[5], com.data);
        bool_value = PROPAGATE_BOOL(res_bool_value);
      }
    } else {

      if (com.condition_variable[0] == '!') {
        result_t res_bool_value =
            get_json_bool(&com.condition_variable[1], com.data);
        bool_value = !PROPAGATE_BOOL(res_bool_value);
      } else {
        result_t res_bool_value =
            get_json_bool(com.condition_variable, com.data);
        bool_value = PROPAGATE_BOOL(res_bool_value);
      }
    }

    if (bool_value) {
      result_t res_cooked_body_value = cook_block_template(
          template_object, com.condition_body, com.data, com.data, 1);
      char *cooked_body_value = PROPAGATE(res_cooked_body_value);

      for (u64 k = 0; k < strlen(cooked_body_value); k++) {
        result_t res_add =
            add_to_buffer(buffer, buffer_size, cooked_body_value[k]);
        PROPAGATE(res_add);
      }

      free(cooked_body_value);
    }
  } else if (com.command_type == PARTIAL) {
    template_t *partial_template = NULL;

    for (u64 i = 0; i < template_object->partials_count; i++) {
      if (strcmp(com.key, template_object->partials[i].partial_name) == 0) {
        partial_template =
            (template_t *)template_object->partials[i].partial_template;
      }
    }

    if (partial_template == NULL) {
      return ERR("no partial named %s found", com.key);
    }

    result_t res_cooked_partial = cook_template(partial_template);
    char *cooked_partial = PROPAGATE(res_cooked_partial);

    for (u64 k = 0; k < strlen(cooked_partial); k++) {
      result_t res_add = add_to_buffer(buffer, buffer_size, cooked_partial[k]);
      PROPAGATE(res_add);
    }

    free(cooked_partial);
  }

  return OK(NULL);
}

result_t cook_template(template_t *template_object) {
  cJSON *data_json = cJSON_Parse(template_object->template_data);
  if (data_json == NULL) {
    return ERR("could not parse template data JSON: `%s`",
               template_object->template_data);
  }

  result_t res_cooked =
      cook_block_template(template_object, template_object->template_string,
                          data_json, data_json, 0);
  char *cooked = PROPAGATE(res_cooked);

  cJSON_Delete(data_json);

  return OK(cooked);
}

result_t cook_block_template(template_t *template_object, char *template_string,
                             void *parent_data, void *block_data, u64 depth) {

  char *buffer = NULL;
  u64 buffer_size = 0;

  char *command_buffer = NULL;
  u64 command_buffer_size = 0;

  bool writing = true;

  for (u64 i = 0; i < strlen(template_string); i++) {
    if (template_string[i] == '{' && template_string[i + 1] == '{') {
      writing = false;
    } else if (i > 0 && template_string[i] == '}' &&
               template_string[i - 1] == '}') {
      writing = true;

      result_t res_add =
          add_to_buffer(&command_buffer, &command_buffer_size, '}');
      PROPAGATE(res_add);

      res_add = add_to_buffer(&command_buffer, &command_buffer_size, '}');
      PROPAGATE(res_add);

      command_buffer[command_buffer_size - 1] = '\0';

      char *command_key = strdup(&command_buffer[2]);
      command_key[strlen(command_key) - 2] = '\0';

      template_command_t com = {0};

      if (command_key[0] == '#') {
        if (strncmp(command_key, "#for", 4) == 0) {
          char *loop_variable = &command_key[5];

          char *loop_body = NULL;
          u64 loop_body_size = 0;

          u64 nested_child_loops = 0;

          while (true) {
            if (strncmp(&template_string[i], "#for", 4) == 0) {
              nested_child_loops++;
            }

            if (template_string[i] == '}' && template_string[i - 1] == '}' &&
                template_string[i - 2] == 'r' &&
                template_string[i - 3] == 'o' &&
                template_string[i - 4] == 'f' &&
                template_string[i - 5] == '/') {

              if (nested_child_loops == 0) {
                loop_body[loop_body_size - 8] = '\0';

                break;
              } else {
                nested_child_loops--;
              }
            }

            loop_body = realloc(loop_body, (loop_body_size + 1) * sizeof(char));
            loop_body[loop_body_size] = template_string[i + 1];

            loop_body_size++;
            i++;
          }

          if (depth == 0) {
            com = (template_command_t){.command_type = FOR_LOOP,
                                       .raw_command = command_buffer,
                                       .data = parent_data,
                                       .key = command_key,

                                       //
                                       .loop_variable = loop_variable,
                                       .loop_body = loop_body};

            result_t res =
                process_command(template_object, &buffer, &buffer_size, com);
            PROPAGATE(res);
          } else {
            com = (template_command_t){.command_type = FOR_LOOP,
                                       .raw_command = command_buffer,
                                       .data = block_data,
                                       .key = command_key,

                                       //
                                       .loop_variable = loop_variable,
                                       .loop_body = loop_body};

            result_t res =
                process_command(template_object, &buffer, &buffer_size, com);
            PROPAGATE(res);
          }
        } else if (strncmp(command_key, "#if", 3) == 0) {
          char *condition_variable = &command_key[4];

          char *condition_body = NULL;
          u64 condition_body_size = 0;

          u64 nested_child_conditions = 0;

          while (true) {
            if (strncmp(&template_string[i], "#if", 3) == 0) {
              nested_child_conditions++;
            }

            if (template_string[i] == '}' && template_string[i - 1] == '}' &&
                template_string[i - 2] == 'f' &&
                template_string[i - 3] == 'i' &&
                template_string[i - 4] == '/') {

              if (nested_child_conditions == 0) {
                condition_body[condition_body_size - 7] = '\0';

                break;
              } else {
                nested_child_conditions--;
              }
            }

            condition_body = realloc(condition_body,
                                     (condition_body_size + 1) * sizeof(char));
            condition_body[condition_body_size] = template_string[i + 1];

            condition_body_size++;
            i++;
          }

          if (depth == 0) {
            com = (template_command_t){.command_type = IF_CONDITION,
                                       .raw_command = command_buffer,
                                       .data = parent_data,
                                       .key = command_key,

                                       //
                                       .condition_variable = condition_variable,
                                       .condition_body = condition_body};

            result_t res =
                process_command(template_object, &buffer, &buffer_size, com);

            free(condition_body);
            PROPAGATE(res);
          } else {
            com = (template_command_t){.command_type = IF_CONDITION,
                                       .raw_command = command_buffer,
                                       .data = block_data,
                                       .key = command_key,

                                       //
                                       .condition_variable = condition_variable,
                                       .condition_body = condition_body};

            result_t res =
                process_command(template_object, &buffer, &buffer_size, com);

            free(condition_body);
            PROPAGATE(res);
          }
        }
      } else if (command_key[0] == '>' ||
                 (strlen(command_key) > 5 &&
                  (strncmp(command_key, "&gt;", 4) == 0))) {

        char *new_key = NULL;

        if (command_key[0] == '>') {
          new_key = &command_key[2];
        } else {
          new_key = &command_key[5];
        }

        com = (template_command_t){.command_type = PARTIAL,
                                   .raw_command = command_buffer,
                                   .data = block_data,
                                   .key = new_key};

        result_t res =
            process_command(template_object, &buffer, &buffer_size, com);
        PROPAGATE(res);
      } else {
        if (command_key[0] == 't' && command_key[1] == 'h' &&
            command_key[2] == 'i' && command_key[3] == 's') {
          com = (template_command_t){.command_type = VALUE,
                                     .raw_command = command_buffer,
                                     .data = block_data,
                                     .key = command_key};

          result_t res =
              process_command(template_object, &buffer, &buffer_size, com);
          PROPAGATE(res);
        } else {
          com = (template_command_t){.command_type = VALUE,
                                     .raw_command = command_buffer,
                                     .data = parent_data,
                                     .key = command_key};

          result_t res =
              process_command(template_object, &buffer, &buffer_size, com);
          PROPAGATE(res);
        }
      }

      free(command_key);

      free(command_buffer);
      command_buffer = NULL;
      command_buffer_size = 0;

      i++;
    }

    if (writing) {
      result_t res_add =
          add_to_buffer(&buffer, &buffer_size, template_string[i]);
      PROPAGATE(res_add);
    } else {
      result_t res_add = add_to_buffer(&command_buffer, &command_buffer_size,
                                       template_string[i]);
      PROPAGATE(res_add);
    }
  }

  result_t res_add = add_to_buffer(&buffer, &buffer_size, '\0');
  PROPAGATE(res_add);

  return OK(buffer);
}
