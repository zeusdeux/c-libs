#ifndef ZDX_JSON_H_
#define ZDX_JSON_H_

#include <stddef.h>

typedef struct json_t json_t;

typedef enum {
  JSON_NULL = 0,
  JSON_NUMBER,
  JSON_BOOLEAN,
  JSON_STRING,
  JSON_PARSE_ERROR,
} json_type_tag;

char *json_type_cstr(json_type_tag jtag);
json_t json_parse(const char *const json_cstr, const size_t sz);

#endif // ZDX_JSON_H_

#ifdef ZDX_JSON_IMPLEMENTATION

#include <stdbool.h>

#define ZDX_SIMPLE_ARENA_IMPLEMENTATION
#include "./zdx_simple_arena.h"

// TODO: setup a wrapper for arena_realloc and #define DA_REALLOC to it here
// You'll have to likely modify arena_realloc implementation to understand a
// special value such a -1 for old_sz to make it possible to even write a
// realloc() compatible wrapper around arena_realloc() for zdx_da.h to consume.
#define ZDX_DA_IMPLEMENTATION
#include "./zdx_da.h"

typedef enum {
  JSON_TOKEN_NULL,
  JSON_TOKEN_NUMBER,
  JSON_TOKEN_BOOLEAN,
  JSON_TOKEN_STRING,
} json_token_type_t;

typedef struct json_t {
  json_token_type_t type;
  union {
    double number;
    bool boolean;
    char *string;
  }
} json_token_t;

typedef struct {
  size_t length;
  size_t capacity;
  json_token_t items[];
} json_lexer_t;

typedef struct json_t {
  json_type_tag type;

  union {
    double number;
    bool boolean;
    char *string;
    char *parse_err;
  }
} json_t;

const char *json_type_cstr(json_type_tag jtag)
{
  static const char *const tag_names[] = {
    "json_null",
    "json_number",
    "json_boolean",
    "json_string",
    "json_parse_error"
  };

  return tag_names[jtag];
}

static json_lexer_t json_lex(arena_t *const ar, const char *const json, size_t sz)
{
  // TODO
}

static json_token_t json_lexer_next_token(json_lexer_t *const lexer)
{
  // TODO
}

json_t json_parse(const char *const json, const size_t sz)
{
  arena_t ar = arena_create(sz * sizeof(json_t) * sizeof(json_lexer_t));

  json_lexer_t lexer = json_lex(&ar, json, sz);
  json_token_t tok;

  while((tok = json_lexer_next_token(&lexer))) {
    // TODO
  }

  arena_free(&ar);
}

#endif // ZDX_JSON_IMPLEMENTATION
