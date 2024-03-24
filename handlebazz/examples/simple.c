#include "../handlebazz.h"
#include <stdlib.h>

int main() {
  char *template_string = "Hello {{name}}, how are you today?";

  char *template_data = "{\"name\": \"world\"}";

  template_t *template_object = create_template(template_string, template_data);

  result_t res_cooked = cook_template(template_object);
  char *cooked = UNWRAP(res_cooked);

  printf("%s \n", cooked);

  free_template(template_object);
  free(cooked);

  return 0;
}
