#include "../handlebazz.h"
#include <stdlib.h>

int main() {
  char *template_string;
  char *template_data;
  template_t *template_object;
  result_t res_cooked;
  char *cooked;

  template_string = "{{#for items}}Hello {{this}}\n{{/for}}";
  template_data = "{\"items\": [1, 2, 3, 4]}";
  template_object = create_template(template_string, template_data);
  res_cooked = cook_template(template_object);
  cooked = UNWRAP(res_cooked);
  printf("%s \n", cooked);
  free_template(template_object);
  free(cooked);

  template_string =
      "{{#for items}}{{this.name}} is {{this.age}} years old\n{{/for}}";
  template_data = "{\"items\": [{\"name\": \"John\", \"age\": 25}, {\"name\": "
                  "\"James\", \"age\": 36}, {\"name\": \"Sam\", \"age\": 19}]}";
  template_object = create_template(template_string, template_data);
  res_cooked = cook_template(template_object);
  cooked = UNWRAP(res_cooked);
  printf("%s \n", cooked);
  free_template(template_object);
  free(cooked);

  return 0;
}
