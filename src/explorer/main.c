#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../json/json.h"
#include "../templating/templating.h"

typedef enum {
  EXPLORER,
  STATS,
} gen_type_t;

result_t gen(char *source_path, char *destination_path, gen_type_t gen_type) {
  result_t res_end_template_string =
      read_file_to_string("./explorer/templates/end.html");
  char *end_template_string = PROPAGATE(res_end_template_string);

  template_t *end_template = create_template(end_template_string, "{}");
  free(end_template_string);

  result_t res_head_template_string =
      read_file_to_string("./explorer/templates/head.html");
  char *head_template_string = PROPAGATE(res_head_template_string);

  char *head_template_data = NULL;

  if (gen_type == EXPLORER) {
    head_template_data =
        "{\"title\": \"Shoggoth Explorer\", \"desc\": \"Shoggoth is a "
        "peer-to-peer network for publishing and distributing open-source "
        "code, Machine Learning models, datasets, and research papers.\", "
        "\"og_url\": \"/explorer/static/img/og/explorer.png\", \"url\": "
        "\"/explorer\", \"is_explorer\": true}";
  } else if (gen_type == STATS) {
    head_template_data =
        "{\"title\": \"Shoggoth Explorer — Stats\", \"desc\": \"Shoggoth is a "
        "peer-to-peer network for publishing and distributing open-source "
        "code, Machine Learning models, datasets, and research papers.\", "
        "\"og_url\": \"/explorer/static/img/og/stats.png\", \"url\": "
        "\"/explorer/stats\", \"is_stats\": true}";
  } else {
    PANIC("unhandled gen type");
  }

  template_t *head_template =
      create_template(head_template_string, head_template_data);
  free(head_template_string);

  result_t res_template_string = read_file_to_string(source_path);
  char *template_string = PROPAGATE(res_template_string);

  template_t *template_object = create_template(template_string, "{}");
  free(template_string);

  template_add_partial(template_object, "head", head_template);
  template_add_partial(template_object, "end", end_template);

  result_t res_cooked_docs = cook_template(template_object);
  char *cooked_docs = PROPAGATE(res_cooked_docs);

  free_template(template_object);
  free_template(head_template);
  free_template(end_template);

  write_to_file(destination_path, cooked_docs, strlen(cooked_docs));

  free(cooked_docs);

  return OK(NULL);
}

int main() {
  printf("Generating explorer ...\n");

  result_t res = gen("./explorer/html/explorer.html",
                     "./explorer/out/explorer.html", EXPLORER);
  UNWRAP(res);

  res = gen("./explorer/html/stats.html", "./explorer/out/stats.html", STATS);
  UNWRAP(res);

  return 0;
}
