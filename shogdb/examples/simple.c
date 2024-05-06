#include "../include/cjson.h"
#include "../src/client/client.h"

int main() {
  NETLIBC_INIT();

  shogdb_ctx_t *db_ctx = new_shogdb("http://127.0.0.1:6961");

  UNWRAP(shogdb_set_int(db_ctx, "my_int", 69));
  db_value_t *res_int = UNWRAP(shogdb_get(db_ctx, "my_int"));
  printf("INT VALUE: %lld\n", res_int->value_int);

  UNWRAP(shogdb_set_uint(db_ctx, "my_uint", 68));
  db_value_t *res_uint = UNWRAP(shogdb_get(db_ctx, "my_uint"));
  printf("UINT VALUE: %lld\n", res_uint->value_uint);

  UNWRAP(shogdb_set_float(db_ctx, "my_float", 68.993));
  db_value_t *res_float = UNWRAP(shogdb_get(db_ctx, "my_float"));
  printf("FLOAT VALUE: %f\n", res_float->value_float);

  UNWRAP(shogdb_set_str(db_ctx, "my_str", "deez nutz"));
  db_value_t *res_str = UNWRAP(shogdb_get(db_ctx, "my_str"));
  printf("STR VALUE: %s\n", res_str->value_str);

  UNWRAP(shogdb_set_bool(db_ctx, "my_bool", 1));
  db_value_t *res_bool = UNWRAP(shogdb_get(db_ctx, "my_bool"));
  printf("BOOL VALUE: %d\n", res_bool->value_bool);

  UNWRAP(shogdb_set_json(db_ctx, "my_json", "[1, 2, 4]"));
  db_value_t *res_json = UNWRAP(shogdb_get(db_ctx, "my_json"));
  printf("JSON VALUE: %s\n", cJSON_Print(res_json->value_json));

  UNWRAP(shogdb_delete(db_ctx, "my_str"));
  
  char *res_all = UNWRAP(shogdb_print(db_ctx));
  printf("ALL VALUES: %s\n", res_all);

  free_shogdb(db_ctx);
}
