#ifndef NETLIBC_LOG_H
#define NETLIBC_LOG_H

#include "../netlibc.h"

typedef enum {
  WARN,
  INFO,
  ERROR,
} log_level_t;

#define RED_COLOR 227, 61, 45
#define YELLOW_COLOR 227, 224, 45

#define EXIT(...) __netlibc_exit(__FILE__, __LINE__, __VA_ARGS__)

#define PANIC(...) __netlibc_panic(__FILE__, __LINE__, __VA_ARGS__)

#define LOG(log_level, ...)                                                    \
  __netlibc_log(log_level, __FILE__, __LINE__, __VA_ARGS__)

void __netlibc_log(log_level_t log_level, const char *file, int line,
                   const char *format, ...);

void __netlibc_panic(const char *file, int line, const char *format, ...);

void __netlibc_exit(const char *file, int line, int status, const char *format,
                    ...);

void set_color_rgb(u8 r, u8 g, u8 b);

void reset_color();

#endif
