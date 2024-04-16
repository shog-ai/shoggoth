/**** og.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

#include "../node.h"

#include <netlibc/fs.h>
#include <netlibc/log.h>
#include <netlibc/mem.h>

unsigned char *ttf_buffer = NULL;
unsigned char *logo_data = NULL;
int logo_width = 0;
int logo_height = 0;
int logo_channels = 0;

#define TTF_BUFFER_SIZE 1 << 20
#define IMG_WIDTH 1132
#define IMG_HEIGHT 566
#define ICON_SIZE 250

#define MARGIN_LEFT 415
#define PADDING_HORIZONTAL 94
#define PADDING_VERTICAL 138

#define LINE_HEIGHT 1.225f

#define BACKGROUND_COLOR_R 134
#define BACKGROUND_COLOR_G 62
#define BACKGROUND_COLOR_B 0

void ellipsis_text(char *text, size_t width) {
  size_t length = strlen(text);
  if (length > width) {
    text[width - 3] = '.';
    text[width - 2] = '.';
    text[width - 1] = '.';
    text[width] = '\0';
  }
}

int count_lines(const char *text) {
  int count = 0;
  for (const char *p = text; *p; p++) {
    if (*p == '\n') {
      count++;
    }
  }
  return count;
}

char *wordwrap(const char *text, size_t width) {
  if (text == NULL || width <= 0) {
    return NULL;
  }

  size_t text_len = strlen(text);
  char *wrapped_text = nmalloc((2 * text_len + 1) * sizeof(char));

  size_t current_width = 0;
  size_t wrapped_text_index = 0;
  size_t i = 0;
  size_t last_break_index = 0;
  while (text[i] != '\0') {
    wrapped_text[wrapped_text_index++] = text[i];
    current_width++;

    if (text[i] == '\n') {
      current_width = 0;
    } else if (current_width >= width) {
      if (text[i] == ' ') {
        last_break_index = i;
        wrapped_text[wrapped_text_index++] = '\n';
        current_width = 0;
      } else {
        size_t j = i;
        while (j > 0 && text[j] != ' ') {
          j--;
        }
        if (j <= last_break_index) {
          last_break_index = i;
          wrapped_text[wrapped_text_index++] = '\n';
          current_width = 0;
        } else {
          last_break_index = i;
          wrapped_text_index -= (i - j);
          i = j;
          wrapped_text[wrapped_text_index++] = '\n';
          current_width = 0;
        }
      }
    }
    i++;
  }
  wrapped_text[wrapped_text_index] = '\0';
  return wrapped_text;
}

void render_line(unsigned char *image, stbtt_fontinfo *font, const char *text,
                 int baseline_y, float pointsize) {
  float scale = stbtt_ScaleForPixelHeight(font, pointsize * LINE_HEIGHT);
  int x = MARGIN_LEFT;

  for (const char *p = text; *p; p++) {
    int w, h, xoff, yoff;
    unsigned char *bitmap =
        stbtt_GetCodepointBitmap(font, scale, scale, *p, &w, &h, &xoff, &yoff);

    if (bitmap) {
      for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
          int image_x = x + i + xoff;
          int image_y = baseline_y + j + yoff;
          if (image_x < 0 || image_x >= IMG_WIDTH || image_y < 0 ||
              image_y >= IMG_HEIGHT)
            continue;
          int byte_index = (image_x + image_y * IMG_WIDTH) * 3;
          unsigned char alpha = bitmap[i + j * w];
          image[byte_index] =
              (unsigned char)((alpha * 255 +
                               (0xFF - alpha) * image[byte_index]) /
                              0xFF);
          image[byte_index + 1] =
              (unsigned char)((alpha * 255 +
                               (0xFF - alpha) * image[byte_index + 1]) /
                              0xFF);
          image[byte_index + 2] =
              (unsigned char)((alpha * 255 +
                               (0xFF - alpha) * image[byte_index + 2]) /
                              0xFF);
        }
      }
    }

    int advance;
    stbtt_GetCodepointHMetrics(font, *p, &advance, NULL);
    x += (int)((float)advance * scale);

    if (bitmap) {
      stbtt_FreeBitmap(bitmap, NULL);
    }
  }
}

void render_wrapped_text(unsigned char *image, stbtt_fontinfo *font,
                         const char *text, int y, float pointsize,
                         size_t wrapat, bool is_gravity_south) {
  char *saveptr;
  char *modified_text = wordwrap(text, wrapat);
  char *line = strtok_r(modified_text, "\n", &saveptr);
  float baseline_y = (float)y - pointsize * (LINE_HEIGHT - 1);
  int i = is_gravity_south ? count_lines(modified_text) : 1;
  while (line != NULL) {
    float y_offset = (float)i * pointsize * LINE_HEIGHT;
    if (is_gravity_south) {
      render_line(image, font, line, (int)(baseline_y - y_offset), pointsize);
      i--;
    } else {
      render_line(image, font, line, (int)(baseline_y + y_offset), pointsize);
      i++;
    }
    line = strtok_r(NULL, "\n", &saveptr);
  }
  nfree(modified_text);
}

unsigned char *generate_og_image(const char *title, const char *desc,
                                 float title_size, size_t title_wrap,
                                 float desc_size, size_t desc_wrap,
                                 int *image_size) {

  stbtt_fontinfo *font = (stbtt_fontinfo *)nmalloc(sizeof(stbtt_fontinfo));
  if (!stbtt_InitFont(font, ttf_buffer,
                      stbtt_GetFontOffsetForIndex(ttf_buffer, 0))) {
    nfree(font);
    unsigned char *dummy_buffer = nmalloc(1);
    dummy_buffer[0] = '\0';
    *image_size = 1;
    return dummy_buffer;
  }

  unsigned char *image = (unsigned char *)nmalloc(IMG_WIDTH * IMG_HEIGHT * 3);
  for (size_t i = 0; i < IMG_WIDTH * IMG_HEIGHT * 3; i += 3) {
    image[i] = BACKGROUND_COLOR_R;
    image[i + 1] = BACKGROUND_COLOR_G;
    image[i + 2] = BACKGROUND_COLOR_B;
  }

  render_wrapped_text(image, font, title, PADDING_VERTICAL, title_size,
                      title_wrap, false);
  render_wrapped_text(image, font, desc, IMG_HEIGHT - PADDING_VERTICAL,
                      desc_size, desc_wrap, true);

  int offset_x = PADDING_HORIZONTAL;
  int offset_y = IMG_HEIGHT / 2 - ICON_SIZE / 2;

  for (int y = 0; y < logo_height; ++y) {
    for (int x = 0; x < logo_width; ++x) {
      int target_x = x + offset_x;
      int target_y = y + offset_y;

      if (target_x >= IMG_WIDTH || target_y >= IMG_HEIGHT || target_x < 0 ||
          target_y < 0)
        continue;

      int png_index = (x + y * logo_width) * logo_channels;
      int image_index = (target_x + target_y * IMG_WIDTH) * 3;

      unsigned char alpha = logo_channels >= 4 ? logo_data[png_index + 3] : 255;

      image[image_index] =
          (unsigned char)((alpha * logo_data[png_index] +
                           (255 - alpha) * image[image_index]) /
                          255);
      image[image_index + 1] =
          (unsigned char)((alpha * logo_data[png_index + 1] +
                           (255 - alpha) * image[image_index + 1]) /
                          255);
      image[image_index + 2] =
          (unsigned char)((alpha * logo_data[png_index + 2] +
                           (255 - alpha) * image[image_index + 2]) /
                          255);
    }
  }

  unsigned char *png_buffer = stbi_write_png_to_mem(
      image, IMG_WIDTH * 3, IMG_WIDTH, IMG_HEIGHT, 3, image_size);
  if (!png_buffer) {
    nfree(image);
    nfree(font);
    unsigned char *dummy_buffer = nmalloc(1);
    dummy_buffer[0] = '\0';
    *image_size = 1;
    return dummy_buffer;
  }

  nfree(image);
  nfree(font);
  return png_buffer;
}

void og_init_stb(node_ctx_t *ctx) {
  ttf_buffer = (unsigned char *)nmalloc(TTF_BUFFER_SIZE);

  char explorer_path[FILE_PATH_SIZE];
  utils_get_node_explorer_path(ctx, explorer_path);

  char font_path[FILE_PATH_SIZE];
  sprintf(font_path, "%s/static/font/Roboto/Roboto-Regular.ttf", explorer_path);

  FILE *fontFile = fopen(font_path, "rb");
  if (!fontFile) {
    PANIC("Error opening font file\n");
  }
  fread(ttf_buffer, 1, TTF_BUFFER_SIZE, fontFile);
  fclose(fontFile);

  char logo_path[FILE_PATH_SIZE];
  sprintf(logo_path, "%s/static/img/icon/icon-250x250.png", explorer_path);

  logo_data =
      stbi_load(logo_path, &logo_width, &logo_height, &logo_channels, 0);
  if (!logo_data) {
    PANIC("Error loading PNG file\n");
  }
}

void og_deinit_stb() {
  nfree(ttf_buffer);
  stbi_image_free(logo_data);
}
