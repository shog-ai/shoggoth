#include <iostream>
#include <jmespath/jmespath.h>

namespace jp = jmespath;

char *jmespath_filter_json(char *json_str, char *filter) {
  jp::Expression expression = filter;

  char *res = jp::search(expression, json_str);

  return res;
}
