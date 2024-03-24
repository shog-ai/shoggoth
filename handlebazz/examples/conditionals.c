#include "../handlebazz.h"
#include <stdlib.h>

int main() {
  char *template_string;
  char *template_data;
  template_t *template_object;
  result_t res_cooked;
  char *cooked;

  template_string = "It is {{#if is_valid}}valid{{/if}}";
  template_data = "{\"is_valid\": false}";
  template_object = create_template(template_string, template_data);
  res_cooked = cook_template(template_object);
  cooked = UNWRAP(res_cooked);
  printf("%s\n", cooked);
  free_template(template_object);
  free(cooked);

  template_string = "It is {{#if is_valid}}valid{{/if}}";
  template_data = "{\"is_valid\": true}";
  template_object = create_template(template_string, template_data);
  res_cooked = cook_template(template_object);
  cooked = UNWRAP(res_cooked);
  printf("%s\n", cooked);
  free_template(template_object);
  free(cooked);

  template_string = "It is {{#if !is_valid}}valid{{/if}}";
  template_data = "{\"is_valid\": true}";
  template_object = create_template(template_string, template_data);
  res_cooked = cook_template(template_object);
  cooked = UNWRAP(res_cooked);
  printf("%s\n", cooked);
  free_template(template_object);
  free(cooked);

  return 0;
}
