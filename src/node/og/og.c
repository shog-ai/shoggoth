#ifdef MAGICKWAND_7
    #include <MagickWand/MagickWand.h>
#else
    #include <wand/MagickWand.h>
#endif
#include <string.h>
#include <stdbool.h>

size_t IMG_WIDTH = 1132;
size_t IMG_HEIGHT = 566;
size_t ICON_SIZE = 250;
size_t PADDING_VERTICAL = 138;
double LINE_HEIGHT = 1.225;

char* wordwrap(const char* text, size_t width) {
    if (text == NULL || width <= 0) {
        return NULL;
    }

    size_t text_len = strlen(text);
    char* wrapped_text = malloc((text_len + text_len / width + 3) * sizeof(char)); // text_width, every line and null-terminator

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

void ellipsis_text(char *text, size_t width) {
    size_t length = strlen(text);
    if (length > width) {
        text[width - 3] = '.';
        text[width - 2] = '.';
        text[width - 1] = '.';
        text[width] = '\0';
    }
}

void add_wrapped_text(MagickWand *main_wand, const char *text, ssize_t pointsize, size_t width, size_t height, ssize_t x, size_t wrap_at, bool is_gravity_south) {
    char *modified_text = wordwrap(text, wrap_at);
    if (modified_text == NULL) {
        // text was empty
        return;
    }

    MagickWand *text_wand = NewMagickWand();
    PixelWand *background_color = NewPixelWand();
    PixelWand *text_color = NewPixelWand();

    PixelSetColor(background_color, "transparent");
    PixelSetColor(text_color, "white");
    MagickNewImage(text_wand, width, height, background_color);

    DrawingWand *draw_wand = NewDrawingWand();
    DrawSetFillColor(draw_wand, text_color);
    DrawSetFontSize(draw_wand, pointsize);
    DrawSetTextAlignment(draw_wand, LeftAlign);

    char *line = strtok(modified_text, "\n");
    ssize_t current_y = pointsize;
    while (line != NULL) {
        MagickAnnotateImage(text_wand, draw_wand, 0, current_y, 0, line);
        current_y += (ssize_t)(pointsize * LINE_HEIGHT);
        line = strtok(NULL, "\n");
    }

    ssize_t text_wand_height = current_y - pointsize;
    ssize_t y_from_bottom = (ssize_t)IMG_HEIGHT - text_wand_height - (ssize_t)PADDING_VERTICAL;
    ssize_t y = is_gravity_south ? y_from_bottom : (ssize_t)PADDING_VERTICAL;
#ifdef MAGICKWAND_7
    MagickCompositeImage(main_wand, text_wand, OverCompositeOp, MagickTrue, x, y);
#else
    MagickCompositeImage(main_wand, text_wand, OverCompositeOp, x, y);
#endif

    free(modified_text);
    DestroyMagickWand(text_wand);
    DestroyPixelWand(background_color);
    DestroyPixelWand(text_color);
    DestroyDrawingWand(draw_wand);
}

unsigned char* generate_og_image(const char* title, const char* desc, ssize_t title_size, size_t title_wrap, ssize_t desc_size, size_t desc_wrap, size_t* image_size) {
    MagickWand *magick_wand;
    DrawingWand *draw_wand;
    PixelWand *background_color;
    PixelWand *text_color;

    magick_wand = NewMagickWand();

    background_color = NewPixelWand();
    PixelSetColor(background_color, "#863e00");
    text_color = NewPixelWand();
    PixelSetColor(text_color, "#ffffff");
    MagickNewImage(magick_wand, IMG_WIDTH, IMG_HEIGHT, background_color);

    draw_wand = NewDrawingWand();
    DrawSetFillColor(draw_wand, text_color);
    DrawSetGravity(draw_wand, NorthWestGravity);

    add_wrapped_text(magick_wand, title, title_size, 750, 300, 415, title_wrap, false);
    add_wrapped_text(magick_wand, desc, desc_size, 750, 300, 415, desc_wrap, true);

    MagickWand *logo_wand = NewMagickWand();
    MagickReadImage(logo_wand, "./node/explorer/static/img/icon/icon-250x250.png");
#ifdef MAGICKWAND_7
    MagickCompositeImage(magick_wand, logo_wand, OverCompositeOp, MagickTrue, 94, IMG_HEIGHT / 2 - ICON_SIZE / 2);
#else
    MagickCompositeImage(magick_wand, logo_wand, OverCompositeOp, 94, IMG_HEIGHT / 2 - ICON_SIZE / 2);
#endif
    DestroyMagickWand(logo_wand);

    MagickSetImageFormat(magick_wand, "PNG");
    unsigned char* png_as_string = NULL;
    *image_size = 0;
    MagickResetIterator(magick_wand);
    png_as_string = MagickGetImageBlob(magick_wand, image_size);

    magick_wand = DestroyMagickWand(magick_wand);
    background_color = DestroyPixelWand(background_color);
    text_color = DestroyPixelWand(text_color);
    draw_wand = DestroyDrawingWand(draw_wand);

    return png_as_string;
}

void og_init_magickwand() {
    MagickWandGenesis();
}

void og_deinit_magickwand() {
    MagickWandTerminus();
}