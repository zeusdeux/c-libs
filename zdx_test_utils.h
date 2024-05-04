#ifndef ZDX_TEST_UTILS_H_
#define ZDX_TEST_UTILS_H_

#include <stdlib.h>

#include "./zdx_util.h"

#define testlog(level, ...) do {                                             \
    const char *zdx_disable_test_output = getenv("ZDX_DISABLE_TEST_OUTPUT"); \
    if (!zdx_disable_test_output) log(level, __VA_ARGS__);                   \
  } while(0)

#endif // ZDX_TEST_UTILS_H_
