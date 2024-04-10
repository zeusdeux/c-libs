/**
 * MIT License
 *
 * Copyright (c) 2024 Mudit
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
#ifndef ZDX_JSON_H_
#define ZDX_JSON_H_

#include <stddef.h>
#include <stdbool.h>
#include "./zdx_simple_arena.h"

/**
 * TODO: ERROR HANDLING ALL OVER FFS (including arena->err, bounds checks, etc)
 * TODO: Handle escape chars such as \u in strings
 * TODO: Handle unicodes in string ffs to report length correctly for example
 * TODO: Support paths in place of key_cstr - for e.g.,
 *   json_object_remove_deep(arena, obj, "a.b.0.c");
 * TODO: Fix the allocation weirdness with having both hardcoded arena_alloc and the HT_CALLOC macro for hashtables
 * TODO: come up with a better way of generating dynamic strings as the snprintf() hidden away in random functions
 *   is weird. Especially given that all snprintf() second param (aka bytes count to copy until) is hardcoded
 *   everywhere snprintf() is used which is gross
 * TODO: WRITE TESTS
 * TODO: Land on main
 * TODO: Figure out how to communicate errors that occur due to out of memory in arena (ERROR HANDLE ALSO JFC)
 */

/* Types and function for JSON parsing */
typedef enum {
  JSON_VALUE_UNEXPECTED = 0,
  JSON_VALUE_NULL,
  JSON_VALUE_NUMBER,
  JSON_VALUE_BOOLEAN,
  JSON_VALUE_STRING,
  JSON_VALUE_ARRAY,
  JSON_VALUE_OBJECT,
  JSON_VALUE_KINDS_COUNT // this should always be last
} json_value_kind_t;

typedef struct json_value_object_t json_value_object_t;

typedef struct json_value_t {
  json_value_kind_t kind;
  union {
    const void *null;
    double number;
    bool boolean;
    struct {
      char *value;
      size_t length;
    } string;
    struct {
      size_t length;
      size_t capacity;
      struct json_value_t *items;
    } array;
    json_value_object_t *object;
  };
  const char *err;
} json_value_t;

typedef struct json_value_object_t {
  size_t length;
  size_t capacity;
  struct json_value_object_kv_t {
    bool occupied;
    size_t key_length;
    const char *key;
    struct json_value_t value;
  } *items;
} json_value_object_t;

/* Types and functions for JSON object type */
typedef struct json_object_return_t json_object_return_t;

json_value_t json_parse(arena_t *const arena, const char *const json_cstr);
json_object_return_t json_object_set(arena_t *const arena, json_value_object_t ht[const static 1], const char *const key_cstr, const json_value_t value);
json_object_return_t json_object_get(const json_value_object_t ht[const static 1], const char *const key_cstr);
json_object_return_t json_object_remove(arena_t *const arena, json_value_object_t ht[const static 1], const char *const key_cstr);
json_object_return_t json_object_get_deep(arena_t *const arena, json_value_object_t ht[const static 1], const char *const key_path_cstr);
json_object_return_t json_object_set_deep(arena_t *const arena, json_value_object_t ht[const static 1], const char *const key_path_cstr, json_value_t value);

#endif // ZDX_JSON_H_

#ifdef ZDX_JSON_IMPLEMENTATION

// TODO: HANDLE ERRORS ALL ACROSS THE CODE IN THIS FILE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "./zdx_util.h"
#define ZDX_STRING_VIEW_IMPLEMENTATION
#include "./zdx_string_view.h"
#undef ZDX_STRING_VIEW_IMPLEMENTATION

typedef enum {
  // err member will carry error string (static with no interpolation of row/col. That's the parser's job to build using the err str)
  // this is also what zero-initializing json_token_t sets the token.kind to. Sane default imho.
  JSON_TOKEN_UNKNOWN = 0,
  // lexer keeps returning end token when done. Making this the first kind so that we can {0} init a json_token_t value
  JSON_TOKEN_END,
  JSON_TOKEN_WS,
  JSON_TOKEN_OCURLY,
  JSON_TOKEN_CCURLY,
  JSON_TOKEN_OSQR,
  JSON_TOKEN_CSQR,
  JSON_TOKEN_COLON,
  JSON_TOKEN_COMMA,
  JSON_TOKEN_SYMBOL, // null, true, false
  // number
  JSON_TOKEN_LONG, // long int and long int in sci notation aka 1e4, 1e+4, 1e-4, etc
  JSON_TOKEN_DOUBLE, // floating point and floating point in sci notation aka 1e4, 1e+4, 1e-4, etc

  JSON_TOKEN_STRING,
} json_token_kind_t;

typedef struct {
  json_token_kind_t kind;
  sv_t value;
  const char *err;
} json_token_t;

typedef struct {
  size_t cursor;
  size_t line;
  size_t bol; // beginning of line. col = bol - cursor
  sv_t *input;
} json_lexer_t;

static const char* json_token_kind_to_cstr(const json_token_kind_t kind)
{
  static const char *const token_kinds[] = {
    "JSON_TOKEN_UNKNOWN",
    "JSON_TOKEN_END",
    "JSON_TOKEN_WS",
    "JSON_TOKEN_OCURLY",
    "JSON_TOKEN_CCURLY",
    "JSON_TOKEN_OSQR",
    "JSON_TOKEN_CSQR",
    "JSON_TOKEN_COLON",
    "JSON_TOKEN_COMMA",
    "JSON_TOKEN_SYMBOL",
    "JSON_TOKEN_LONG",
    "JSON_TOKEN_DOUBLE",
    "JSON_TOKEN_STRING",
  };

  return token_kinds[kind];
}

static json_token_t json_lexer_next_token(json_lexer_t *const lexer)
{
  json_token_t token = {0};

  // TODO: add assertions for values in lexer struct that make sense here

  if (lexer->input->length == 0 || lexer->cursor >= lexer->input->length) {
    token.kind = JSON_TOKEN_END;
    return token;
  }

  if (isspace(lexer->input->buf[lexer->cursor])) {
    token.kind = JSON_TOKEN_WS;
    token.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    if (lexer->input->buf[lexer->cursor] == '\n') {
      lexer->line += 1;
      lexer->bol = lexer->cursor + 1;
    }

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == '{') {
    token.kind = JSON_TOKEN_OCURLY;
    token.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == '}') {
    token.kind = JSON_TOKEN_CCURLY;
    token.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == '[') {
    token.kind = JSON_TOKEN_OSQR;
    token.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == ']') {
    token.kind = JSON_TOKEN_CSQR;
    token.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == ':') {
    token.kind = JSON_TOKEN_COLON;
    token.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == ',') {
    token.kind = JSON_TOKEN_COMMA;
    token.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  // null, true, false
  if (isalpha(lexer->input->buf[lexer->cursor])) {
    token.kind = JSON_TOKEN_SYMBOL;
    token.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;

    // ok to check rather than assert cursor < input.length since the already
    // consumed character is a valid token and hence we have something to return
    // even when this loop doesn't run.
    while(lexer->cursor < lexer->input->length && isalpha(lexer->input->buf[lexer->cursor])) {
      token.value.length += 1;
      lexer->cursor += 1;
    }
    return token;
  }

  // Instead of asserts we instead return JSON_TOKEN_UNKNOWN with captured unknown chars in string view so that
  // parser can show useful errors.

  // numbers
  int curr_char_isdigit = isdigit(lexer->input->buf[lexer->cursor]);
  int curr_char_isplus = lexer->input->buf[lexer->cursor] == '+';
  int curr_char_isminus = lexer->input->buf[lexer->cursor] == '-';

  if (curr_char_isdigit || curr_char_isplus || curr_char_isminus) {
    if (curr_char_isdigit) {
      token.kind = JSON_TOKEN_LONG;
      token.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 1); // consume <digit>

      lexer->cursor += 1; // move cursor past <digit>
    }
    else if (curr_char_isplus || curr_char_isminus) {
      if ((lexer->cursor + 1) >= lexer->input->length || !isdigit(lexer->input->buf[lexer->cursor + 1])) {
        token.kind = JSON_TOKEN_UNKNOWN;
        token.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 1); // consume (<plus> | <minus>)
        token.err = "Expected a digit to follow";

        lexer->cursor += 1; // move cursor past (<plus> | <minus>)
        return token;
      }

      token.kind = JSON_TOKEN_LONG;
      token.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 2); // consume (<plus> | <minus>)<digit>
                                                                     //
      lexer->cursor += 2; // move cursor past (<plus> | <minus>)<digit>
    }
    else {
      assertm(false, "JSON LEXER: UNREACHABLE: Expected number to start with digit, '+' or '-'");
    }

    // TODO: due to this loop, we even lex "00.2313" as double which should technically fail parsing
    while(lexer->cursor < lexer->input->length && isdigit(lexer->input->buf[lexer->cursor])) {
      token.value.length += 1;
      lexer->cursor += 1;
    }

    // floating point component
    if (lexer->cursor < lexer->input->length && lexer->input->buf[lexer->cursor] == '.') {
      if ((lexer->cursor + 1) >= lexer->input->length || !isdigit(lexer->input->buf[lexer->cursor + 1])) {
        token.kind = JSON_TOKEN_UNKNOWN;
        token.value.length += 1; // consume <dot>
        token.err = "Expected a digit to follow";

        lexer->cursor += 1; // move cursor past <dot>
        return token;
      }

      token.kind = JSON_TOKEN_DOUBLE;
      token.value.length += 2; // consume <dot><digit>
      lexer->cursor += 2; // move cursor past <dot><digit>

      while(lexer->cursor < lexer->input->length && isdigit(lexer->input->buf[lexer->cursor])) {
        token.value.length += 1;
        lexer->cursor += 1;
      }
    }

    // exponent component
    if (lexer->cursor < lexer->input->length && (lexer->input->buf[lexer->cursor] == 'e' || lexer->input->buf[lexer->cursor] == 'E')) {
      if ((lexer->cursor + 1) >= lexer->input->length) {
          token.kind = JSON_TOKEN_UNKNOWN;
          token.value.length += 1; // consume (<e> | <E>)
          token.err = "Expected a '+', '-' or a digit to follow";

          lexer->cursor += 1; // move cursor past (<e> | <E>)
          return token;
        }


      int next_char_isdigit = isdigit(lexer->input->buf[lexer->cursor + 1]);
      int next_char_isplus = lexer->input->buf[lexer->cursor + 1] == '+';
      int next_char_isminus = lexer->input->buf[lexer->cursor + 1] == '-';

      if (next_char_isdigit) {
        token.value.length += 2; // consume <e><digit>
        lexer->cursor += 2; // move cursor past <e><digit>
      }
      else if (next_char_isplus || next_char_isminus) {
        if ((lexer->cursor + 2) >= lexer->input->length || !isdigit(lexer->input->buf[lexer->cursor + 2])) {

          token.kind = JSON_TOKEN_UNKNOWN;
          token.value.length += 2; // consume (<e> | <E>)(<plus> | <minus>)
          token.err = "Expected a digit to follow";

          lexer->cursor += 2; // move cursor past (<e> | <E>)(<plus> | <minus>)
          return token;
        }

        token.value.length += 3; // consume <e>(<plus> | <minus>)<digit>
        lexer->cursor += 3; // move cursor past <e>(<plus> | <minus>)<digit>
      }
      else {
        token.kind = JSON_TOKEN_UNKNOWN;
        token.value.length += 1;
        token.err = "Expected a '+', '-' or a digit to follow";

        lexer->cursor += 1; // move cursor past (<e> | <E>)
        return token;
      }

      while (lexer->cursor < lexer->input->length && isdigit(lexer->input->buf[lexer->cursor])) {
        token.value.length += 1;
        lexer->cursor += 1;
      }
    }

    return token; // valid LONG or DOUBLE token
  }

  // strings - the tokenized string doesn't contain surrounding dbl quotes to make it easier on the parser
  if (lexer->input->buf[lexer->cursor] == '"') {
    token.kind = JSON_TOKEN_STRING;
    lexer->cursor += 1; // move cursor past OPENING <dbl_quote>
    // don't consume anything as yet as the next char might be a " (aka dbl quote)
    // denoting end of this string in which case, token.value.buf should be empty and token.value.length should be 0
    token.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 0);

    while(true) {
      // end of input before end of string so return JSON_TOKEN_UNKNOWN
      if (lexer->cursor >= lexer->input->length) {
        token.kind = JSON_TOKEN_UNKNOWN;
        token.err = "Expected a closing quote (\")";
        return token;
      }

      if (lexer->input->buf[lexer->cursor] == '"' && lexer->input->buf[lexer->cursor - 1] != '\\') {
        token.kind = JSON_TOKEN_STRING;

        lexer->cursor += 1; // move cursor past CLOSING <dbl_quote> without consuming it in the string token value
        return token;
      }

      token.value.length += 1;
      lexer->cursor += 1;
    }

    assertm(false, "JSON LEXER: UNREACHABLE: Expected JSON_TOKEN_STRING or JSON_TOKEN_UNKNOWN to have already been returned");
  }

  token.kind = JSON_TOKEN_UNKNOWN; // this is default but I'm hardcoding just in case I re-order the json_token_type_t enum again
  token.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);
  token.err = "Unrecognised token";

  lexer->cursor += 1;
  return token;
}

static json_token_t json_lexer_peek_token(json_lexer_t *const lexer)
{
  json_lexer_t before = {
    .cursor = lexer->cursor,
    .line = lexer->line,
    .bol = lexer->bol,
    .input = lexer->input
  };

  json_token_t tok = json_lexer_next_token(lexer);

  lexer->cursor = before.cursor;
  lexer->line = before.line;
  lexer->bol = before.bol;
  lexer->input = before.input;

  return tok;
}

static const char *json_value_kind_to_cstr(json_value_kind_t kind)
{
  static const char *const value_kinds[] = {
    "JSON_VALUE_UNEXPECTED",
    "JSON_VALUE_NULL",
    "JSON_VALUE_NUMBER",
    "JSON_VALUE_BOOLEAN",
    "JSON_VALUE_STRING",
    "JSON_VALUE_ARRAY",
    "JSON_VALUE_OBJECT",
  };

  assertm(zdx_arr_len(value_kinds) == JSON_VALUE_KINDS_COUNT,
          "Non-exhaustive use of enum: All json value kinds not handled");
  return value_kinds[kind];
}

// TODO: should this return void? or perhaps bool? keeping as void for
// now as return value of snprintf() is weird af.
// snprintf() and vsnprintf() return the number of characters that
// would have been printed if the size were unlimited (again, not
// including the final ‘\0’)
static void json_value_by_kind_as_cstr(json_value_t json, char *out_cstr, const size_t max_out_cstr_sz)
{
  switch (json.kind) {
  case JSON_VALUE_UNEXPECTED: {
    snprintf(out_cstr, max_out_cstr_sz, "Unexpected value (err: %s)",
             json.err);
  } break;
  case JSON_VALUE_NULL: {
    snprintf(out_cstr, max_out_cstr_sz, "null (err: %s)",
             json.err);
  } break;
  case JSON_VALUE_NUMBER: {
    snprintf(out_cstr, max_out_cstr_sz, "%lf (err: %s)",
             json.number, json.err);
  } break;
  case JSON_VALUE_BOOLEAN: {
    snprintf(out_cstr, max_out_cstr_sz, "%s (err: %s)",
             json.boolean ? "true" : "false",
             json.err);
  } break;
  case JSON_VALUE_STRING: {
    snprintf(out_cstr, max_out_cstr_sz, "%s (err: %s)",
             json.string.value,
             json.err);
  } break;
  case JSON_VALUE_ARRAY: {
    snprintf(out_cstr, max_out_cstr_sz, "array [addr %p length %zu cap %zu err: %s]",
             (void *)json.array.items, json.array.length, json.array.capacity,
             json.err);
  } break;
  case JSON_VALUE_OBJECT: {
    snprintf(out_cstr, max_out_cstr_sz, "object [addr %p length %zu cap %zu err: %s]",
             (void *)json.object, json.object->length, json.object->capacity,
             json.err);
  } break;
  default: {
    snprintf(out_cstr, max_out_cstr_sz, "Unrecognized json value (err: %s)",
             json.err);
  } break;
  }
  out_cstr[max_out_cstr_sz] = 0;
}

static json_value_t json_build_unexpected(arena_t *const arena, json_lexer_t *const lexer, json_token_t tok, const char *const err_prefix)
{
  json_value_t jv = {0};

  assertm(err_prefix && strlen(err_prefix) > 0,
          "Expected an error message but received %s (%p)", err_prefix, (void *)err_prefix);
  size_t sz = strlen(err_prefix) + 128;
  char *err = arena_calloc(arena, 1, sz);

  snprintf(err, sz, "%s at line %zu and col %zu",
           err_prefix,
           lexer->line + 1 /* line is 0 index */,
           lexer->cursor - lexer->bol - tok.value.length + 1 /* +1 for managing 0 index */);

  jv.kind = JSON_VALUE_UNEXPECTED;
  jv.err = err;

  return jv;
}

static json_value_t json_parse_symbol(arena_t *const arena, json_lexer_t *const lexer)
{
  json_value_t jv = {0};
  json_token_t tok = json_lexer_next_token(lexer);

  assertm(tok.kind == JSON_TOKEN_SYMBOL, "Expected a symbol token but received %s", json_token_kind_to_cstr(tok.kind));

  if (sv_eq_cstr(tok.value, "null")) {
    jv.kind = JSON_VALUE_NULL;
    jv.null = NULL;
  }
  else if (sv_eq_cstr(tok.value, "true")) {
    jv.kind = JSON_VALUE_BOOLEAN;
    jv.boolean = true;
  }
  else if (sv_eq_cstr(tok.value, "false")) {
    jv.kind = JSON_VALUE_BOOLEAN;
    jv.boolean = false;
  }
  else {
    const char* err_prefix = "Invalid symbol";
    size_t sz = strlen(err_prefix) + tok.value.length + 16; // 16 bytes are YOLO padding just in case
    char *err = arena_calloc(arena, 1, sz);

    snprintf(err, sz, "%s '"SV_FMT"'",
             err_prefix,
             sv_fmt_args(tok.value));

    jv = json_build_unexpected(arena, lexer, tok, err);
  }

  return jv;
}

static json_value_t json_parse_number(arena_t *const arena, json_lexer_t *const lexer)
{
  json_value_t jv = {0};
  json_token_t tok = json_lexer_next_token(lexer);

  assertm(tok.kind == JSON_TOKEN_LONG || tok.kind == JSON_TOKEN_DOUBLE,
          "Expected a long or double token but received %s", json_token_kind_to_cstr(tok.kind));

  char *str = arena_calloc(arena, 1, tok.value.length + 1);
  memcpy(str, tok.value.buf, tok.value.length);

  // TODO: ERROR HANDLING
  jv.number = strtod(str, NULL); // strtoll doesn't parse exponent form hence using only strtod which does
  jv.kind = JSON_VALUE_NUMBER;

  return jv;
}

// TODO: handle unicode and other escapes that expand to strings
static json_value_t json_parse_string(arena_t *const arena, json_lexer_t *const lexer)
{
  json_value_t jv = {0};
  json_token_t tok = json_lexer_next_token(lexer);

  assertm(tok.kind == JSON_TOKEN_STRING, "Expected a string token but received %s", json_token_kind_to_cstr(tok.kind));

  char *str = arena_calloc(arena, 1, tok.value.length + 1); // + 1 for \0

  /**
   * We don't add a \0 at the end as we use arena_calloc and
   * thus init whole string + 1 with \0 before memcpy-ing
   **/
  memcpy(str, tok.value.buf, tok.value.length);

  jv.kind = JSON_VALUE_STRING;
  jv.string.value = str;
  jv.string.length = tok.value.length;

  return jv;
}

static json_value_t json_parse_unknown(arena_t *const arena, json_lexer_t *const lexer)
{
  json_token_t tok = json_lexer_next_token(lexer);

  assertm(tok.kind == JSON_TOKEN_UNKNOWN, "Expected an unknown token but received %s", json_token_kind_to_cstr(tok.kind));
  assertm(tok.err, "Expected unknown token to carry an error but received %s (%p)", tok.err, (void *)tok.err);

  size_t sz = strlen(tok.err) + tok.value.length + 16; // 16 bytes of padding just in case we run out of space
  char *err = arena_calloc(arena, 1, sz);

  snprintf(err, sz, "%s '"SV_FMT"'",
           tok.err,
           sv_fmt_args(tok.value));

  return json_build_unexpected(arena, lexer, tok, err);
}

#ifndef JSON_DA_MIN_CAPACITY
#define JSON_DA_MIN_CAPACITY 8
#endif // JSON_DA_MIN_CAPACITY

#define json_da_push(arena, arr, item) {                                                                          \
    while(((arr)->length + 1) > (arr)->capacity) {                                                                \
      size_t new_sz = (arr)->capacity ? ((arr)->capacity * 2) : JSON_DA_MIN_CAPACITY;                             \
      (arr)->items = arena_realloc((arena), (arr)->items, (arr)->length * sizeof(item), new_sz * sizeof((item))); \
                                                                                                                  \
      if ((arena)->err) {                                                                                         \
        bail("Error: %s", (arena)->err);                                                                          \
      }                                                                                                           \
      (arr)->capacity = new_sz;                                                                                   \
                                                                                                                  \
      dbg("++ arr->items %p \t| arr->capacity %zu \t| arr->length %zu \t| sizeof(item) %zu\n",                    \
          (void *)(arr)->items, (arr)->capacity, (arr)->length, sizeof((item)));                                  \
    }                                                                                                             \
                                                                                                                  \
    (arr)->items[(arr)->length++] = (item);                                                                       \
  } while(0)

#ifndef JSON_MAX_DEPTH
#define JSON_MAX_DEPTH  256
#endif // JSON_MAX_DEPTH

// forward declaration as json_parse_array() calls it
static json_value_t json_parse_object(arena_t *const arena, json_lexer_t *const lexer);

static json_value_t json_parse_array(arena_t *const arena, json_lexer_t *const lexer)
{
  json_value_t jvs[JSON_MAX_DEPTH ] = {0};
  long int jvs_length = 0; // as this can go negative and long as size_t is unsigned long

  json_token_t tok = json_lexer_next_token(lexer);
  assertm(tok.kind == JSON_TOKEN_OSQR, "Expected an opening '[' but received %s", json_token_kind_to_cstr(tok.kind));

  while(true) {
    if (tok.kind == JSON_TOKEN_OSQR) {
      // we are trying to go another level deeper hence this check
      if (jvs_length >= JSON_MAX_DEPTH ) {
        return json_build_unexpected(arena, lexer, tok,
                                     "Reached maximum depth of nested arrays"
                                     " (consider reducing depth or configuring JSON_MAX_DEPTH  macro)");
      }
      jvs[jvs_length++] = (json_value_t){
        .kind = JSON_VALUE_ARRAY
      };
    }
    else if (tok.kind == JSON_TOKEN_CSQR) {
      if (jvs_length <= 0) {
        return json_build_unexpected(arena, lexer, tok, "Unexpected array close with no array opened");
      }

      if (jvs_length == 1) {
        return jvs[0];
      }

      /* insert the nested array into the parent if we have any nested arrays (denoted by jvs_length > 1) */
      if (jvs_length > 1) {
        json_da_push(arena, &jvs[jvs_length - 2].array, jvs[jvs_length - 1]);
      }

      jvs_length -= 1;
    }
    else if (tok.kind == JSON_TOKEN_END) {
      return json_build_unexpected(arena, lexer, tok, "Unexpected end of input while parsing an array");
    }
    else {
      // TODO: figure out how to do this switch case better
      switch (tok.kind) {
      case JSON_TOKEN_WS:
      case JSON_TOKEN_COMMA:
        break;
      case JSON_TOKEN_SYMBOL: {
        json_da_push(arena, &jvs[jvs_length - 1].array, json_parse_symbol(arena, lexer));
      } break;

      case JSON_TOKEN_LONG:
      case JSON_TOKEN_DOUBLE: {
        json_da_push(arena, &jvs[jvs_length - 1].array, json_parse_number(arena, lexer));
      } break;

      case JSON_TOKEN_STRING: {
        json_da_push(arena, &jvs[jvs_length - 1].array, json_parse_string(arena, lexer));
      } break;

      case JSON_TOKEN_OCURLY: {
        json_value_t value = json_parse_object(arena, lexer);
        json_da_push(arena, &jvs[jvs_length - 1].array, value);
      } break;

      case JSON_TOKEN_UNKNOWN: {
        json_da_push(arena, &jvs[jvs_length - 1].array, json_parse_unknown(arena, lexer));
      } break;

      default: {
        assertm(false,
                "JSON PARSER: UNREACHABLE: While parsing array, cannot parse unknown token kind '"SV_FMT"'",
                sv_fmt_args(tok.value));
      } break;
      }

    }
    tok = json_lexer_peek_token(lexer);

    /* consume these tokens but leave the other "proper" value tokens lexer for their respective parsers to consume */
    if (tok.kind == JSON_TOKEN_OSQR ||
        tok.kind == JSON_TOKEN_CSQR ||
        tok.kind == JSON_TOKEN_WS   ||
        tok.kind == JSON_TOKEN_COMMA) {
      tok = json_lexer_next_token(lexer);
    }
  }

  assertm(false, "JSON PARSER: UNREACHABLE: While parsing array, something broke out of the token consumption loop");
}

/* -------------- HASHTABLE -------------- */
static size_t hash_djb2(const char *key, size_t sz)
{
  size_t hash = 5381;
  size_t k = 33;

  for (size_t i = 0; i < sz; i++) {
    hash = hash * k ^ (size_t)key[i];
  }

  return hash;
}

static size_t hash_fnv1(const char *key, size_t sz)
{
  size_t hash = 14695981039346656037UL;

  for (size_t i = 0; i < sz; i++) {
    hash = (hash * 1099511628211UL) ^ (size_t)key[i];
  }

  return hash;
}

typedef struct json_object_return_t {
  json_value_t value;
  const char* err;
} json_object_return_t;

#define ht_has_collision(item, key, sz)         \
  (item)->occupied &&                           \
  ((item)->key_length != (sz) ||                \
   *(item)->key != *(key) ||                    \
   memcmp((item)->key, (key), (sz)))

static size_t ht_get_index(const json_value_object_t ht[const static 1], const char *const key, const size_t sz)
{
  size_t idx = hash_djb2(key, sz) % ht->capacity;

  if (ht_has_collision(&ht->items[idx], key, sz)) {
    idx = hash_fnv1(key, sz) % ht->capacity;
  }

  size_t k = 1;
  size_t max_k = 128;

  while(ht_has_collision(&ht->items[idx], key, sz)) {
    idx += (k * k);
    idx = idx % ht->capacity;
    k = (k + 2) % max_k; // k + 2 instead of k << 2 as for some keys, k << 2 would result in an infinite loop here
  }

  return idx;
}

#define ht_dbg(label, ht) dbg("%s length %zu \t| capacity %zu \t| items %p", \
                              (label), (ht)->length, (ht)->capacity, (void *)(ht)->items)

#ifndef HT_CALLOC
// the first arg override is to support passing an arena for an arena allocator based alloc fn
// same for the last arg as that's needed by an arena allocator based realloc fn
#define HT_CALLOC(arena, count, size) ((void)((arena)), calloc((count), (size)))
#endif // HT_CALLOC

#ifndef HT_FREE
// can be a noop for an arena based allocator
#define HT_FREE(ptr) free((ptr))
#endif // HT_FREE

#ifndef HT_MIN_LOAD_FACTOR
#define HT_MIN_LOAD_FACTOR 0.125
#endif // HT_MIN_LOAD_FACTOR 0

#ifndef HT_MAX_LOAD_FACTOR
#define HT_MAX_LOAD_FACTOR 0.8
#endif // HT_MAX_LOAD_FACTOR 0

#ifndef HT_MIN_CAPACITY
#define HT_MIN_CAPACITY 32
#endif // HT_MIN_CAPACITY

#define ht_resize(arena, ht)                                                                                  \
  do {                                                                                                        \
    float load_factor = (ht)->capacity ? (ht)->length / (float)(ht)->capacity : 0;                            \
    /* if items aren't initialized, resize to base size */                                                    \
    if ((ht)->items == NULL) {                                                                                \
      load_factor = 0;                                                                                        \
    }                                                                                                         \
    dbg("++ load factor %0.4f (min: %0.4f max: %0.4f)", load_factor, HT_MIN_LOAD_FACTOR, HT_MAX_LOAD_FACTOR); \
                                                                                                              \
    if (HT_MIN_LOAD_FACTOR < load_factor && load_factor < HT_MAX_LOAD_FACTOR) {                               \
      break;                                                                                                  \
    }                                                                                                         \
                                                                                                              \
    size_t moved = 0;                                                                                         \
                                                                                                              \
    struct json_value_object_kv_t *old_items = (ht)->items;                                                   \
    size_t old_length = (ht)->length;                                                                         \
    size_t old_capacity = (ht)->capacity;                                                                     \
                                                                                                              \
    /* shrink */                                                                                              \
    if (load_factor <= HT_MIN_LOAD_FACTOR) {                                                                  \
      if ((ht)->capacity == HT_MIN_CAPACITY) break;                                                           \
      size_t new_sz = (ht)->capacity ? (size_t)((ht)->capacity * 0.5f) : HT_MIN_CAPACITY;                     \
      new_sz = new_sz < HT_MIN_CAPACITY ? HT_MIN_CAPACITY : new_sz;                                           \
      dbg("++ shrinking to %zu", new_sz);                                                                     \
      (ht)->items = HT_CALLOC((arena), new_sz, sizeof(*(ht)->items));                                         \
      (ht)->capacity = new_sz;                                                                                \
    }                                                                                                         \
    /* grow */                                                                                                \
    else if (load_factor >= HT_MAX_LOAD_FACTOR) {                                                             \
      size_t new_sz = (ht)->capacity ? (ht)->capacity * 2 : HT_MIN_CAPACITY;                                  \
      dbg("++ growing to %zu", new_sz);                                                                       \
      (ht)->items = HT_CALLOC((arena), new_sz, sizeof(*(ht)->items));                                         \
      (ht)->capacity = new_sz;                                                                                \
    }                                                                                                         \
    else {                                                                                                    \
      assertm(false, "JSON OBJECT HASHTABLE: UNREACHABLE: load factor %0.4f", load_factor);                   \
    }                                                                                                         \
                                                                                                              \
    assertm((ht)->items != NULL, "Expected items to be a valid ptr but received %p", (void *)(ht)->items);    \
    assertm((ht)->capacity > 0, "Expected capacity to be > 0, Received: %zu", (ht)->capacity);                \
                                                                                                              \
    if (old_items != (ht)->items && old_items) {                                                              \
      for (size_t i = 0; i < old_capacity; i++) {                                                             \
        struct json_value_object_kv_t *item = &old_items[i];                                                  \
                                                                                                              \
        if (!item->occupied) {                                                                                \
          continue;                                                                                           \
        }                                                                                                     \
                                                                                                              \
        size_t idx = ht_get_index(ht, item->key, item->key_length);                                           \
        (ht)->items[idx].key = item->key;                                                                     \
        (ht)->items[idx].key_length = item->key_length;                                                       \
        (ht)->items[idx].value = item->value;                                                                 \
        (ht)->items[idx].occupied = item->occupied;                                                           \
        moved++;                                                                                              \
      }                                                                                                       \
                                                                                                              \
      HT_FREE(old_items);                                                                                     \
    }                                                                                                         \
                                                                                                              \
    assertm(moved == old_length, "Expected to move %zu elements but instead moved %zu", old_length, moved);   \
    (ht)->length = old_length;                                                                                \
  } while(0)

/* -------------- HASHTABLE USED IN JSON OBJECTS -------------- */
#define json_object_set(arena, ht, key, value) json_object_set_((arena), (ht), (key), strlen((key)), (value))
json_object_return_t json_object_set_(arena_t *const arena,
                                      json_value_object_t ht[const static 1],
                                      const char *const key,
                                      const size_t key_sz,
                                      const json_value_t value)
{
  ht_dbg(">>", ht);
  ht_resize(arena, ht);

  size_t idx = ht_get_index(ht, key, key_sz);

  dbg("~~ idx %zu \t| key %.*s \t| key length %zu", idx, (int)key_sz, key, key_sz);

  if (!ht->items[idx].occupied) {
    ht->length++;
  }

  ht->items[idx].key = key;
  ht->items[idx].key_length = key_sz;
  ht->items[idx].occupied = true;
  ht->items[idx].value = value;

  ht_dbg("<<", ht);
  return (json_object_return_t) {
    .value = value
  };
}

#define json_object_get(ht, key_cstr) json_object_get_((ht), (key_cstr), strlen((key_cstr)))
static json_object_return_t json_object_get_(const json_value_object_t ht[const static 1], const char *const key, const size_t key_sz)
{
  ht_dbg(">>", ht);

  if (!ht->items) {
    return (json_object_return_t) {
      .err = "Key not found (empty object)"
    };
  }

  size_t idx = ht_get_index(ht, key, key_sz);

  if (!ht->items[idx].occupied) {
    return (json_object_return_t) {
      .err = "Key not found"
    };
  }

  ht_dbg("<<", ht);
  return (json_object_return_t) {
    .value = ht->items[idx].value
  };
}

#define json_object_remove(ht, key_cstr) json_object_remove_(ht, key_cstr, strlen((key_cstr)))
static inline json_object_return_t json_object_remove_(json_value_object_t ht[const static 1], const char *const key, const size_t key_sz)
{
  ht_dbg(">>", ht);

  size_t idx = ht_get_index(ht, key, key_sz);
  json_object_return_t ret = {0};

  dbg("~~ idx %zu \t| key %s \t| occupied %s", idx, key, ht->items[idx].occupied ? "true" : "false");

  if (ht->items[idx].occupied) {
    ret.value = ht->items[idx].value;

    ht->items[idx] = (struct json_value_object_kv_t){0};
    ht->length--;
  }

  ht_dbg("<<", ht);
  return ret;
}

json_object_return_t json_object_get_deep(arena_t *const arena, json_value_object_t ht[const static 1], const char *const key_path_cstr)
{
  ht_dbg(">>", ht);
  json_object_return_t result = {0};
  json_value_t json = {0};
  json.object = ht;

  sv_t key_path_sv = sv_from_cstr(key_path_cstr);
  sv_t key_sv = sv_split_by_char(&key_path_sv, '.');

  while(key_sv.buf) {
    result = json_object_get_(json.object, key_sv.buf, key_sv.length);

    if (result.err) {
      break;
    }

    // key_path_sv is updated to remaining string on each call to sv_split_by_char
    // the key_path_sv.length check let's us know if we have another key coming up
    // as checking if the value is an object should only happen until last key - 1
    // as the last key in the path can refer to any type of json value
    if (key_path_sv.length && result.value.kind != JSON_VALUE_OBJECT) {
      char *val_cstr = arena_calloc(arena, 128, sizeof(*val_cstr));
      json_value_by_kind_as_cstr(result.value, val_cstr, 128);

      sv_t next_key_sv = sv_split_by_char(&key_path_sv, '.');

      char *err = arena_calloc(arena, 256, sizeof(*err));
      snprintf(err, 256, "Failed to lookup key '"SV_FMT"' at key '"SV_FMT"' Expected: object, Received: %s",
               sv_fmt_args(next_key_sv), sv_fmt_args(key_sv), val_cstr);
      err[256] = 0;
      result.err = err;
      result.value = json;
      break;
    }

    json.object = result.value.object;
    key_sv = sv_split_by_char(&key_path_sv, '.');
  }

  dbg("<< value %s \t| err %s", json_value_by_kind_as_cstr(result.value), result.err);
  return result;
}

/* json_object_return_t json_object_remove_deep(arena_t *const arena, json_value_object_t ht[const static 1], const char *const key_path_cstr) */
/* { */
/*   ht_dbg(">>", ht); */
/*   json_object_return_t result = {0}; */
/*   json_value_t json = {0}; */

/*   json.object = ht; */

/*   sv_t key_path_sv = sv_from_cstr(key_path_cstr); */
/*   sv_t key_sv = sv_split_by_char(&key_path_sv, '.'); */

/*   while(key_sv.buf) { */
/*     result = json_object_get_(json.object, key_sv.buf, key_sv.length); */

/*     if (result.err){} */
/*   } */


/*   dbg("<< value %s \t| err %s", json_value_by_kind_as_cstr(result.value), result.err); */
/*   return result; */
/* } */


// TODO: This is stupid slow. Fix it and ship a MUCH faster version!
// But this will always remain slower than json_object_set so make sure to
// document that in usage.
json_object_return_t json_object_set_deep(arena_t *const arena, json_value_object_t ht[const static 1], const char *const key_path_cstr, json_value_t value)
{
  ht_dbg(">>", ht);

  sv_t key_path_sv = sv_from_cstr(key_path_cstr);
  sv_t key_sv = sv_split_by_char(&key_path_sv, '.');
  sv_t last_key_sv = {0};
  json_value_object_t *parent = ht;

  while(key_sv.buf) {
    sv_t next_key_sv = sv_split_by_char(&key_path_sv, '.');
    dbg(">>> key_sv "SV_FMT" (len %zu)\n", sv_fmt_args(key_sv), key_sv.length);
    dbg(">>> next_key_sv "SV_FMT" (len %zu)\n", sv_fmt_args(next_key_sv), next_key_sv.length);

    if (next_key_sv.buf) {
      json_object_return_t ret_obj = json_object_get_(parent, key_sv.buf, key_sv.length);

      if (ret_obj.err) {
        json_value_object_t *obj = arena_calloc(arena, 1, sizeof(*obj));
        ret_obj = json_object_set_(arena, parent,
                                  key_sv.buf, key_sv.length,
                                  (json_value_t){ .kind = JSON_VALUE_OBJECT, .object = obj });
      }

      if (ret_obj.value.kind != JSON_VALUE_OBJECT) {
        char *err = arena_calloc(arena, 128, sizeof(*err));
        snprintf(err, 128, "Cannot set key '"SV_FMT"' on key '"SV_FMT"' of type %s as it is not an object",
                 sv_fmt_args(next_key_sv), sv_fmt_args(key_sv), json_value_kind_to_cstr(ret_obj.value.kind));
        return (json_object_return_t){ .err = err };
      }

      parent = ret_obj.value.object;
    }

    last_key_sv = key_sv;
    key_sv = next_key_sv;
  }

  if (!parent->capacity || !parent->items) {
    ht_resize(arena, parent);
  }

  size_t idx = ht_get_index(parent, last_key_sv.buf, last_key_sv.length);

  dbg("~~ idx %zu \t| key %s \t| key length %zu \t| value (kind %s)",
         idx, last_key_sv.buf, last_key_sv.length, json_value_kind_to_cstr(value.kind));

  if (!parent->items[idx].occupied) {
    parent->length++;
  }

  char *key = arena_calloc(arena,last_key_sv.length + 1, sizeof(*key));
  memcpy(key, last_key_sv.buf, last_key_sv.length);
  key[last_key_sv.length] = 0;
  parent->items[idx].key = key;
  parent->items[idx].key_length = last_key_sv.length;
  parent->items[idx].occupied = true;

  if (value.kind == JSON_VALUE_OBJECT && !value.object) {
    value.object = arena_calloc(arena, 1, sizeof(*value.object));
    ht_resize(arena, value.object);
  }
  parent->items[idx].value = value;

  ht_dbg("<<", parent);
  return (json_object_return_t) {
    .value = value
  };
}

static json_value_t json_parse_object(arena_t *const arena, json_lexer_t *const lexer)
{
  json_value_t jvs[JSON_MAX_DEPTH] = {0};
  char *keys[JSON_MAX_DEPTH] = {0};
  long int jvs_length = 0; // as this can go negative and long as size_t is unsigned long
  char *key = NULL; // we insert only one key, value pair at once

  json_token_t tok = json_lexer_next_token(lexer);
  assertm(tok.kind == JSON_TOKEN_OCURLY, "Expected an opening '{' but received %s", json_token_kind_to_cstr(tok.kind));

  while(true) {
    if (tok.kind == JSON_TOKEN_OCURLY) {
      // we are trying to go another level deeper hence this check
      if (jvs_length >= JSON_MAX_DEPTH) {
        return json_build_unexpected(arena, lexer, tok,
                                     "Reached maximum depth of nested objects"
                                     " (consider reducing depth or configuring JSON_MAX_DEPTH macro)");
      }
      json_value_object_t *object = arena_calloc(arena, 1, sizeof(*object));

      if (arena->err) {
        return json_build_unexpected(arena, lexer, tok, "Could not allocated memory for a new object");
      }

      if (key) {
        keys[jvs_length - 1] = key; // map key to new object that was just "opened" using "{"
        key = NULL; // track keys for the internal object we are now parsing
      }

      jvs[jvs_length++] = (json_value_t){
        .kind = JSON_VALUE_OBJECT,
        .object = object
      };
    }
    else if (tok.kind == JSON_TOKEN_CCURLY) {
      if (jvs_length <= 0) {
        return json_build_unexpected(arena, lexer, tok, "Unexpected object close with no object opened");
      }

      if (jvs_length == 1) {
        if (key) {
          char *err = arena_calloc(arena, 128, 1);

          if (arena->err) {
            snprintf(err, 128, "Could not allocate memory for a new object due to %s", arena->err);
            err[128] = 0; // hard truncate error string at 128 chars
            return json_build_unexpected(arena, lexer, tok, "Could not allocated memory for a new object");
          }
          snprintf(err, 128, "Malformed object: No value for key '%s'", key);
          err[128] = 0; // hard truncate error string at 128 chars
          return json_build_unexpected(arena, lexer, tok, err);
        }
        return jvs[0];
      }

      /* insert the nested array into the parent if we have any nested arrays (denoted by jvs_length > 1) */
      if (jvs_length > 1) {
        json_value_t value = jvs[jvs_length - 1];
        if (key) {
          char *err = arena_calloc(arena, 128, 1);
          if (arena->err) {
            snprintf(err, 128, "Could not allocate memory for a new object due to %s", arena->err);
            err[128] = 0; // hard truncate error string at 128 chars
            return json_build_unexpected(arena, lexer, tok, "Could not allocated memory for a new object");
          }
          snprintf(err, 128, "Malformed object: No value for key '%s'", key);
          err[128] = 0; // hard truncate error string at 128 chars
          value = json_build_unexpected(arena, lexer, tok, err);
        }
        json_object_set(arena, jvs[jvs_length - 2].object, keys[jvs_length - 2], value);
        key = NULL;
      }

      jvs_length -= 1;
    }
    else if (tok.kind == JSON_TOKEN_END) {
      return json_build_unexpected(arena, lexer, tok, "Unexpected end of input while parsing an object");
    }
    else {
      // TODO: figure out how to do this switch case better
      switch (tok.kind) {
      case JSON_TOKEN_WS:
      case JSON_TOKEN_COMMA:
        break;
      case JSON_TOKEN_COLON: {
        if (!key) {
          return json_build_unexpected(arena, lexer, tok, "Unexpected ':' token while parsing an object");
        }
      } break;

      case JSON_TOKEN_OSQR: {
        if (!key) {
          return json_build_unexpected(arena, lexer, tok, "Unexpected '[' token while parsing an object");
        } else {
          json_object_set(arena, jvs[jvs_length - 1].object, key, json_parse_array(arena, lexer));
          key = NULL;
        }
      } break;

      case JSON_TOKEN_SYMBOL: {
        if (!key) {
          return json_build_unexpected(arena, lexer, tok, "Unexpected symbol token while parsing an object");
        } else {
          json_object_set(arena, jvs[jvs_length - 1].object, key, json_parse_symbol(arena, lexer));
          key = NULL;
        }
      } break;

      case JSON_TOKEN_LONG: {
        if (!key) {
          return json_build_unexpected(arena, lexer, tok, "Unexpected long token while parsing an object");
        } else {
          json_object_set(arena, jvs[jvs_length - 1].object, key, json_parse_number(arena, lexer));
          key = NULL;
        }
      } break;

      case JSON_TOKEN_DOUBLE: {
        if (!key) {
          return json_build_unexpected(arena, lexer, tok, "Unexpected double token while parsing an object");
        } else {
          json_object_set(arena, jvs[jvs_length - 1].object, key, json_parse_number(arena, lexer));
          key = NULL;
        }
      } break;

      case JSON_TOKEN_STRING: {
        if (!key) {
          json_value_t json_string = json_parse_string(arena, lexer);
          key = json_string.string.value;
        } else {
          json_object_set(arena, jvs[jvs_length - 1].object, key, json_parse_string(arena, lexer));
          key = NULL;
        }
      } break;

      case JSON_TOKEN_UNKNOWN: {
        return json_build_unexpected(arena, lexer, tok, "Unexpected unknown token while parsing an object");
      } break;

      default: {
        assertm(false,
                "JSON PARSER: UNREACHABLE: While parsing object, cannot parse unknown token kind '"SV_FMT"'",
                sv_fmt_args(tok.value));
      } break;
      }
    }

    tok = json_lexer_peek_token(lexer);

    /* consume these tokens but leave the other "proper" value tokens lexer for their respective parsers to consume */
    if (tok.kind == JSON_TOKEN_OCURLY ||
        tok.kind == JSON_TOKEN_CCURLY ||
        tok.kind == JSON_TOKEN_WS   ||
        tok.kind == JSON_TOKEN_COMMA ||
        tok.kind == JSON_TOKEN_COLON) {
      tok = json_lexer_next_token(lexer);
    }
  }

  assertm(false, "JSON PARSER: UNREACHABLE: While parsing object, something broke out of the token consumption loop");
}

json_value_t json_parse(arena_t *const arena, const char *const json_cstr)
{
  json_value_t jv = {0};
  sv_t input = sv_from_cstr(json_cstr);
  json_lexer_t lexer = {
    .input = &input
  };


  // get token without changing lexer state
  // as it's on the json_parse_* functions to
  // consume the token and move the lexer forward
  json_token_t tok = json_lexer_peek_token(&lexer);

  // ignore leading whitespace
  while(tok.kind == JSON_TOKEN_WS) {
    json_lexer_next_token(&lexer); // move lexer forward
    // get first token without changing lexer state aka don't consume token
    // as the json_parse_* function below do that
    tok = json_lexer_peek_token(&lexer);
  }

  switch (tok.kind) {
  case JSON_TOKEN_SYMBOL: {
    jv = json_parse_symbol(arena, &lexer);
  } break;

  case JSON_TOKEN_LONG:
  case JSON_TOKEN_DOUBLE: {
    jv = json_parse_number(arena, &lexer);
  } break;

  case JSON_TOKEN_STRING: {
    jv = json_parse_string(arena, &lexer);
  } break;

  case JSON_TOKEN_UNKNOWN: {
    jv = json_parse_unknown(arena, &lexer);
  } break;

  case JSON_TOKEN_OSQR: {
    jv = json_parse_array(arena, &lexer);
  } break;

  case JSON_TOKEN_OCURLY: {
    jv = json_parse_object(arena, &lexer);
  } break;

  case JSON_TOKEN_END: return json_build_unexpected(arena, &lexer, tok, "Unexpected end of input");

  default: {
    assertm(false, "JSON PARSER: UNREACHABLE: Cannot parse unknown token kind '"SV_FMT"'", sv_fmt_args(tok.value));
  } break;
  }

  // consume next token as no more json_parse_* calls below
  tok = json_lexer_next_token(&lexer);

  // ignore trailing whitespace
  while(tok.kind == JSON_TOKEN_WS) {
    tok = json_lexer_next_token(&lexer); // consume whitespace token as no more json_parse_* calls below
  }

  if (tok.kind != JSON_TOKEN_END) {
    const size_t sz = 128;
    char *err = arena_calloc(arena, 1, sz);

    snprintf(err, sz, "Expected end of input but received '"SV_FMT"' (%s)",
             sv_fmt_args(tok.value),
             json_token_kind_to_cstr(tok.kind));

    return json_build_unexpected(arena, &lexer, tok, err);
  }

  return jv;
}

#endif // ZDX_JSON_IMPLEMENTATION
