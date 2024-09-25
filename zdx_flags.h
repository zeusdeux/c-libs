/**
 * MIT License
 *
 * Copyright (c) 2024 Mudit Ameta
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#ifndef ZDX_FLAGS_H_
#define ZDX_FLAGS_H_

#include <stddef.h>
#include <stdbool.h>

#ifdef ZDX_FLAGS_IMPLEMENTATION_AUTO
#  define ZDX_STRING_VIEW_IMPLEMENTATION
#  define ZDX_ERROR_IMPLEMENTATION
#  define ZDX_FLAGS_IMPLEMENTATION
#  define ZDX_DA_IMPLEMENTATION
#endif

#include "zdx_string_view.h"
#include "zdx_error.h"
#include "zdx_da.h"
#include "zdx_util.h"

typedef struct {
  sv_t key;
  sv_t value;
} flag_t;

typedef struct {
  size_t length;
  size_t capacity;
  flag_t *items;
} flags_t;

typedef enum {
  FLAG_TYPE_STRING,
  FLAG_TYPE_BOOLEAN,
  FLAG_TYPE_INT32,
  FLAG_TYPE_INT64,
  FLAG_TYPE_FLOAT,
  FLAG_TYPE_DOUBLE,
  FLAG_TYPE_STRING_ARRAY,
  FLAG_TYPE_COUNT,
} flag_type_t;

typedef struct {
  err_t err;
  flag_type_t kind;
  union {
    bool boolean;
    int32_t int32;
    int64_t int64;
    float flt;
    double dbl;
    sv_t sv;
    struct {
      size_t length;
      size_t capacity;
      sv_t *items;
    } svs;
  } as;
} flag_value_t;

typedef struct {
  const char* name;
  const char* alias;
  const char* help;
  flag_type_t type;
} flag_option_t;


err_t flags_parse(flags_t flags[const static 1], int argc, char **argv);
flag_value_t flags_get(flags_t flags[const static 1], flag_option_t option);
void flags_deinit(flags_t flags[const static 1]);
void flag_value_deinit(flag_value_t flag[const static 1]);

// ----------------------------------------------------------------------------------------------------------------

#ifdef ZDX_FLAGS_IMPLEMENTATION

static char *trim_left(char *str, char c)
{
  while(*str == c) str++;
  return str;
}

err_t flags_parse(flags_t flags[const static 1], int argc, char **argv)
{
  if (argc < 2) {
    dbg("<< Nothing to parse as argc lesser than 2 (argc = %d)", argc);
    return err_create("Too few arguments. Expected greated than 2");
  }

  // logging
  for (int i = 0; i < argc; i++) {
    dbg("<< argv[%d]: %s", i, argv[i]);
  }

  // parse flags
  char *key = NULL;
  for (int i = 1; i < argc; i++) {
    // if it looks like a flag, make it active key
    if (argv[i][0] == '-') {
      // if an active key already exists, save its value as ""
      // aka default value of any key
      if (key != NULL) {
        da_push(flags, (flag_t){
            .key = sv_from_cstr(key),
            .value = sv_from_cstr(""),
          });
      }

      key = trim_left(argv[i], '-');
    } else {
      // if it looks like a value, if there's an active key,
      // push this value as its value and clear active key
      // else return an error
      char *value = argv[i];

      if (key == NULL) {
        dbg("<< No flag provided for value \"%s\"", value);
        return err_create("No flag provided for value");
      } else {
        da_push(flags, (flag_t){
            .key = sv_from_cstr(key),
            .value = sv_from_cstr(value),
          });
      }
      key = NULL;
    }
  }

  // there's an active key pending, set its value to default ("")
  if (key != NULL) {
    da_push(flags, (flag_t){
        .key = sv_from_cstr(key),
        .value = sv_from_cstr(""),
      });
    key = NULL;
  }

  // logging
  // TODO(mudit): Figure out how to remove all of this in release builds
  for (size_t i = 0; i < flags->length; i++) {
    dbg("<< Flag (key = \""SV_FMT"\" (%p) value \""SV_FMT"\" (%p))",
        sv_fmt_args(flags->items[i].key), (void *)flags->items[i].key.buf,
        sv_fmt_args(flags->items[i].value), (void *)flags->items[i].value.buf);
  }

  return err_none;
}

flag_value_t flags_get(flags_t flags[const static 1], flag_option_t option)
{
  flag_value_t result = {0};

  dbg(">> Flags (length = %zu)", flags->length);
  dbg(">> Option (name = %s, alias = %s, type = %d)", option.name, option.alias, option.type);

  bool found_match = false;

  for (size_t i = 0; i < flags->length; i++) {
    flag_t flag = flags->items[i];

    dbg(">> Flag (key = \""SV_FMT"\", value = \""SV_FMT"\")", sv_fmt_args(flag.key), sv_fmt_args(flag.value));

    if (sv_eq_cstr(flag.key, option.name) || sv_eq_cstr(flag.key, option.alias)) {
      found_match = true;

      switch (option.type) {
        case FLAG_TYPE_BOOLEAN: {
          result.as.boolean = flag.value.buf != NULL;
          result.kind = FLAG_TYPE_BOOLEAN;

          return result;
        } break;

        case FLAG_TYPE_STRING: {
          result.as.sv = flag.value;
          result.kind = FLAG_TYPE_STRING;

          return result;
        } break;

        case FLAG_TYPE_STRING_ARRAY: {
          result.kind = FLAG_TYPE_STRING_ARRAY;
          da_push(&result.as.svs, flag.value);
        } break;

        default: {
          dbg("<< Invalid flag type %d", option.type);

          return (flag_value_t){
            .err = err_create("Invalid flag type")
          };
        }
      }
    }
  }

  // This is the return for FLAG_TYPE_STRING_ARRAY
  // as its only for that type that we go through all
  // parsed flags (to collect them in a dynamic array).
  // result here is result.kind = FLAG_TYPE_STRING_ARRAY
  if (found_match) {
    return result;
  }

  dbg("<< Not found Option (name = %s, alias = %s, type = %d)", option.name, option.alias, option.type);

  // boolean flags always contain values and default to false
  if (option.type == FLAG_TYPE_BOOLEAN) {
    dbg("<< Defaulting boolean flag \"%s\" to false", option.name);
    return (flag_value_t){ .kind = FLAG_TYPE_BOOLEAN };
  }

  return (flag_value_t){
    .err = err_create("Flag not found")
  };
}

void flags_deinit(flags_t flags[const static 1])
{
  da_deinit(flags);
}

void flag_value_deinit(flag_value_t flag[const static 1])
{
  if (!err_exists(&flag->err)) {
    switch(flag->kind) {
      case FLAG_TYPE_STRING_ARRAY: {
        da_deinit(&flag->as.svs);
      } break;
      default: break;
    }
  }
}

#endif // ZDX_FLAGS_IMPLEMENTATION
#endif // ZDX_FLAGS_H_
