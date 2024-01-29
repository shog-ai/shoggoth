/**** tuwi.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Tuwi framework, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../tuwi.h"

#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/ioctl.h>
#else
#include <windows.h>
#endif

#include <unistd.h>

#ifndef _WIN32
cords_t tuwi_get_terminal_size() {
  struct winsize view_size;
  ioctl(0, TIOCGWINSZ, &view_size);

  cords_t cords = {view_size.ws_col, view_size.ws_row};

  return cords;
}
#else
cords_t tuwi_get_terminal_size() {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

  cords_t cords = {csbi.srWindow.Right - csbi.srWindow.Left + 1,
                   csbi.srWindow.Bottom - csbi.srWindow.Top + 1};

  return cords;
}
#endif

bool tuwi_terminal_resized(cords_t former_size) {
  cords_t size = tuwi_get_terminal_size();

  if (former_size.row != size.row || former_size.col != size.col) {
    return true;
  }

  return false;
}

void tuwi_fill_canvas(rgb_t color, cords_t anchor, u32 width, u32 height,
                      bool auto_width, bool auto_height) {
  if (auto_width) {
    width = (tuwi_get_terminal_size().col * width) / 100;
  }

  if (auto_height) {
    height = (tuwi_get_terminal_size().row * height) / 100;
  }

  for (u32 i = 0; i < height; i++) {
    for (u32 k = 0; k < width; k++) {
      tuwi_set_bgcolor_rgb(color);
      tuwi_put_char(anchor.row + i, anchor.col + k, ' ');
    }
  }
}

void tuwi_disable_mouse_reporting() { printf("\033[?1000l"); }

void tuwi_enable_mouse_reporting() { printf("\033[?1000h"); }

rgb_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  rgb_t new_rgb = {.r = r, .g = g, .b = b};

  return new_rgb;
};

void tuwi_reset_terminal() {
  printf("\ec");
  tuwi_flush_terminal();
}

void tuwi_flush_terminal() { fflush(stdout); }

void tuwi_clear_terminal() {
  printf("\ec");
  tuwi_flush_terminal();
}

void tuwi_put_char(uint64_t line, uint64_t column, wchar_t ch) {
  tuwi_set_cursor(line, column);
  putwchar(ch);
}

void tuwi_put_str_raw(uint64_t line, uint64_t column, char *str) {
  tuwi_set_cursor(line, column);

  printf("%s", str);
}

void tuwi_put_str(uint64_t line, uint64_t column, char *str) {
  for (uint32_t k = 0; k < strlen(str); k++) {
    tuwi_put_char(line, column + k, str[k]);
  }
}

void tuwi_clear_line(uint64_t line) {
  tuwi_set_cursor(line, 0);

  printf("\e[2K");
}

void tuwi_set_cursor_to_row_start() { printf("\x1b[G"); }

void tuwi_clear_current_line() {
  printf("\e[2K");

  tuwi_set_cursor_to_row_start();
}

void tuwi_clear_col(uint64_t line, uint64_t col) {
  tuwi_set_cursor(line, col);

  putchar(' ');
}

void tuwi_set_cursor(uint64_t line, uint64_t col) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
  printf("\e[%lu;%luH", line, col);
#pragma GCC diagnostic pop
}

void tuwi_set_cursor_shape(cursor_shape_t shape) {
  switch (shape) {
  case SHAPE_DEFAULT:
    printf("\e[0 q");
    break;
  case SHAPE_BLOCK:
    printf("\e[2 q");
    break;
  case SHAPE_UNDERLINE:
    printf("\e[4 q");
    break;
  case SHAPE_BAR:
    printf("\e[6 q");
    break;
  default:
    printf("Invalid cursor shape");
  }
}

void tuwi_set_cursor_style(cursor_style_t style) {
  switch (style) {
  case STYLE_NORMAL:
    printf("\e[?25h");
    break;
  case STYLE_BLINKING:
    printf("\e[?12h");
    break;
  case STYLE_STEADY:
    printf("\e[?12l");
    break;
  case STYLE_INVISIBLE:
    printf("\e[?25l");
    break;
  default:
    printf("Invalid cursor style");
  }
}

void tuwi_set_row_bgcolor_rgb(uint64_t row, uint64_t row_size, rgb_t color) {
  printf("\e[48;2;%d;%d;%dm", color.r, color.g, color.b);

  tuwi_set_bgcolor_rgb(color);

  for (uint32_t i = 1; i < row_size + 1; i++) {
    tuwi_put_char(row, i, ' ');
  }
}

void tuwi_set_bgcolor_rgb(rgb_t color) {
  printf("\e[48;2;%d;%d;%dm", color.r, color.g, color.b);
}

void tuwi_set_color_rgb(rgb_t color) {
  printf("\e[38;2;%d;%d;%dm", color.r, color.g, color.b);
}

void tuwi_set_color(color_t color) {
  switch (color) {

  case BLACK:
    printf("\e[30m");
    break;

  case GREEN:
    printf("\e[32m");
    break;

  case YELLOW:
    printf("\e[33m");
    break;

  case MAGENTA:
    printf("\e[35m");
    break;

  case CYAN:
    printf("\e[36m");
    break;

  case WHITE:
    printf("\e[37m");
    break;

  case RED:
    printf("\e[31m");
    break;
  case BLUE:
    printf("\e[34m");
    break;

  case DEFAULT:
    printf("\e[0m");
    break;
  }
}

void tuwi_reset_color() { tuwi_set_color(DEFAULT); }

void tuwi_reset_background_color() { tuwi_set_background_color(DEFAULT); }

void tuwi_set_background_color(color_t color) {
  switch (color) {

  case BLACK:
    printf("\e[40m");
    break;

  case GREEN:
    printf("\e[42m");
    break;

  case YELLOW:
    printf("\e[43m");
    break;

  case MAGENTA:
    printf("\e[45m");
    break;

  case CYAN:
    printf("\e[46m");
    break;

  case WHITE:
    printf("\e[47m");
    break;

  case RED:
    printf("\e[41m");
    break;
  case BLUE:
    printf("\e[44m");
    break;

  case DEFAULT:
    printf("\e[0m");
    break;
  }
}
