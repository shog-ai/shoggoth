#include "../handlebazz.h"
#include <stdlib.h>

int main() {
  char *partial_template_string = "my name is {{name}}";
  char *partial_template_data = "{\"name\": \"John Doe\"}";
  template_t *partial_template_object =
      create_template(partial_template_string, partial_template_data);

  char *template_string = "Hello {{person}}, {{> my_partial}}";
  char *template_data = "{\"person\": \"James\"}";
  template_t *template_object = create_template(template_string, template_data);

  template_add_partial(template_object, "my_partial", partial_template_object);

  result_t res_cooked = cook_template(template_object);
  char *cooked = UNWRAP(res_cooked);

  printf("%s \n", cooked);

  free_template(template_object);
  free(cooked);

  return 0;
}
