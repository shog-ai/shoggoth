#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/cjson.h"
#include "../json/json.h"
#include "../md_to_html/md_to_html.h"
#include "../templating/templating.h"
#include "../utils/utils.h"

#include "docs_api.h"

typedef enum {
  DOCS,
  SONIC,
  CAMEL,
  TUWI,
} gen_type_t;

result_t gen(char *source_path, char *destination_path, gen_type_t gen_type) {
  result_t res_end_template_string =
      read_file_to_string("./explorer/templates/end.html");
  char *end_template_string = PROPAGATE(res_end_template_string);

  template_t *end_template = create_template(end_template_string, "{}");
  free(end_template_string);

  result_t res_table_of_contents_template_string =
      read_file_to_string("./explorer/templates/table_of_contents.html");
  char *table_of_contents_template_string = PROPAGATE(res_end_template_string);

  template_t *table_of_contents_template =
      create_template(table_of_contents_template_string, "{}");
  free(table_of_contents_template_string);

  result_t res_head_template_string =
      read_file_to_string("./explorer/templates/head.html");
  char *head_template_string = PROPAGATE(res_head_template_string);

  char *head_data = NULL;
  if (gen_type == DOCS) {
    head_data = "{\"title\": \"Shoggoth Documentation\", \"is_docs\": true}";
  } else if (gen_type == SONIC) {
    head_data =
        "{\"title\": \"Sonic - Shoggoth Documentation\", \"is_docs\": true}";
  } else if (gen_type == CAMEL) {
    head_data =
        "{\"title\": \"Camel - Shoggoth Documentation\", \"is_docs\": true}";
  } else if (gen_type == TUWI) {
    head_data =
        "{\"title\": \"Tuwi - Shoggoth Documentation\", \"is_docs\": true}";
  } else {
    PANIC("unhandled gen type");
  }

  template_t *head_template = create_template(head_template_string, head_data);
  free(head_template_string);

  result_t res_docs_template_string = read_file_to_string(source_path);
  char *docs_template_string = PROPAGATE(res_docs_template_string);

  template_t *docs_template = create_template(docs_template_string, "{}");
  free(docs_template_string);

  template_add_partial(docs_template, "head", head_template);
  template_add_partial(docs_template, "end", end_template);
  template_add_partial(docs_template, "table_of_contents", end_template);

  result_t res_cooked_docs = cook_template(docs_template);
  char *cooked_docs = PROPAGATE(res_cooked_docs);

  free_template(docs_template);
  free_template(head_template);
  free_template(end_template);

  write_to_file(destination_path, cooked_docs, strlen(cooked_docs));

  free(cooked_docs);

  md_file_to_html_file(destination_path, destination_path);

  return OK(NULL);
}

int main() {
  printf("Generating docs ...\n");

  result_t res =
      gen("./explorer/docs/md/tuwi.md", "./explorer/out/docs/tuwi.html", TUWI);
  UNWRAP(res);

  res = gen("./explorer/docs/md/sonic.md", "./explorer/out/docs/sonic.html",
            SONIC);
  UNWRAP(res);

  res = gen("./explorer/docs/md/camel.md", "./explorer/out/docs/camel.html",
            CAMEL);
  UNWRAP(res);

  res =
      gen("./explorer/docs/md/docs.md", "./explorer/out/docs/docs.html", DOCS);
  UNWRAP(res);

  res = gen_api_docs();
  UNWRAP(res);

  return 0;
}