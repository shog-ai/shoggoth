#ifndef NODE_OG_H
#define NODE_OG_H

void ellipsis_text(char *text, size_t width);
unsigned char* generate_og_image(const char* title, const char* desc, float title_size, size_t title_wrap, float desc_size, size_t desc_wrap, int* image_size);
void og_init_stb();
void og_deinit_stb();

#endif