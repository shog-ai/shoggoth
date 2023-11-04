#ifndef MD2H_GEN
#define MD2H_GEN

typedef enum {
  HEAD_DOCS,
  HEAD_API,
} head_type_t;

typedef enum {
  END_DOCS,
} end_type_t;

result_t md_file_to_html_file(char *input_path, char *output_path);

result_t generate_html(char *input);

#endif
