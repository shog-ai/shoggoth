#include <netlibc/log.h>

#include "../../include/cjson.h"
#include "../client/client.h"

int main(int argc, char **argv) {
  NETLIBC_INIT();

  if (argc < 2) {
    PANIC("no argument supplied");
  }

  char *address = argv[1];

  shogdb_ctx_t *db_ctx = new_shogdb(address);

  char input[100];

  do {
    // Prompt the user for input
    printf("> ");

    // Read the input
    fgets(input, sizeof(input), stdin);

    // Remove the trailing newline character
    input[strcspn(input, "\n")] = '\0';

    if (strlen(input) < 3) {
      printf("invalid command - too short `%s`\n", input);
      continue;
    }

    // Check if the user wants to exit
    if (strcmp(input, "exit") == 0) {
      break;
    } else if (strcmp(input, "help") == 0) {
      printf("Commands:\n");
      printf("help - show this screen\n");
      printf("exit - exit the program\n");
      printf("print - print all the keys and their values\n");
      printf("address - show the server address\n");
      printf("\n");
    } else if (strcmp(input, "print") == 0) {
      char *res_all = UNWRAP(shogdb_print(db_ctx));
      printf("%s\n", res_all);
    } else if (strcmp(input, "address") == 0) {
      printf("%s\n", address);
    } else if (strncmp(input, "GET ", 4) == 0) {
      char *key = &input[4];

      db_value_t *res = UNWRAP(shogdb_get(db_ctx, key));
      char *res_str = shogdb_print_value(res);

      printf("%s\n", res_str);
    } else if (strncmp(input, "SET ", 4) == 0) {
      // char *key = &input[4];

      // db_value_t *res = UNWRAP(shogdb_get(db_ctx, key));
      // char *res_str = shogdb_print_value(res);

      printf("OK\n");
    } else {
      printf("invalid command `%s`\n", input);
    }
  } while (1);

  printf("Exiting ...\n");

  free_shogdb(db_ctx);

  return 0;
}
