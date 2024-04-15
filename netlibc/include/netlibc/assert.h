#ifndef NETLIBC_ASSERT_H
#define NETLIBC_ASSERT_H

#include "../netlibc.h"

void __netlibc_assert_true(bool condition, char *error_message, char *file,
                           int line);

void __netlibc_assert_false(bool condition, char *error_message, char *file,
                            int line);

#ifndef NDEBUG

#define ASSERT_TRUE(condition, message)                                        \
  __netlibc_assert_true(condition, message, __FILE__, __LINE__)

#define ASSERT_FALSE(condition, message)                                       \
  __netlibc_assert_false(condition, message, __FILE__, __LINE__)

#endif

#ifdef NDEBUG
#define ASSERT_TRUE(condition, message)
#define ASSERT_FALSE(condition, message)
#endif

#endif
