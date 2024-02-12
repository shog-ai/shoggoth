#include "../tuwi.h"

#include <stdio.h>
#include <unistd.h>

int main() {
  printf("\nDoing something cool...\n\n");

  tuwi_set_cursor_style(STYLE_INVISIBLE);

  for (int i = 0; i < 10; i++) {
    tuwi_clear_current_line();
    tuwi_flush_terminal();

    printf("PROGRESS: ");

    tuwi_set_background_color(GREEN);

    for (int k = 0; k < i; k++) {
      printf("   ");
    }

    tuwi_reset_color();

    for (int k = 0; k < 10 - i; k++) {
      printf("   ");
    }

    printf("%d%%", (i * 10) + 10);

    tuwi_flush_terminal();
    sleep(1);
  }

  tuwi_set_cursor_style(STYLE_NORMAL);

  printf("\n");

  tuwi_flush_terminal();
  tuwi_reset_color();
  return 0;
}
