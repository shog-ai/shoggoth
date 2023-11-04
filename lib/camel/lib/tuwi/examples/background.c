#include "../tuwi.h"

#include <stdio.h>

const char *message = "The quick brown fox jumps over the lazy dog";

#define PRINT() printf("%s", message)

int main() {
  tuwi_set_background_color(YELLOW);

  tuwi_set_color(BLACK);

  PRINT();

  tuwi_flush_terminal();
  tuwi_reset_color();
  return 0;
}
