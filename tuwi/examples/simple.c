#include "../tuwi.h"

#include <stdio.h>

const char *message = "The quick brown fox jumps over the lazy dog \n";

#define PRINT() printf("%s\n", message)

int main() {
  PRINT();

  tuwi_set_color(GREEN);
  PRINT();

  tuwi_set_color(YELLOW);
  PRINT();

  tuwi_set_color(MAGENTA);
  PRINT();

  tuwi_set_color(CYAN);
  PRINT();

  tuwi_set_color(WHITE);
  PRINT();

  tuwi_set_color(BLUE);
  PRINT();

  tuwi_set_color(RED);
  PRINT();

  tuwi_set_color(BLACK);
  PRINT();

  tuwi_set_color_rgb(rgb(99, 99, 99));
  PRINT();

  tuwi_set_color_rgb(rgb(252, 136, 3));
  PRINT();

  tuwi_set_color_rgb(rgb(157, 252, 3));
  PRINT();

  tuwi_reset_color();
  return 0;
}
