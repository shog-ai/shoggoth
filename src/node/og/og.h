#ifndef NODE_OG_H
#define NODE_OG_H

unsigned char* generate_og_image(const char* title, const char* desc, ssize_t title_size, size_t title_wrap, ssize_t desc_size, size_t desc_wrap, size_t* image_len);

#endif