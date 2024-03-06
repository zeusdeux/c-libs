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
#include "./zdx_simple_arena.h"

typedef struct json_t json_t;

typedef enum {
  JSON_VALUE_UNKNOWN = 0,
  JSON_VALUE_NULL,
  JSON_VALUE_NUMBER,
  JSON_VALUE_BOOLEAN,
  JSON_VALUE_STRING,
} json_value_type_t;

/* PARSER */
const char *json_value_type_to_cstr(json_value_type_t value);
json_t json_parse(arena_t *const arena, const char *const json_cstr, const size_t sz);

#endif // ZDX_JSON_H_

#ifdef ZDX_JSON_IMPLEMENTATION

#include <stdbool.h>
#include <ctype.h>

#include "./zdx_util.h"

#define ZDX_SIMPLE_ARENA_IMPLEMENTATION
#include "./zdx_simple_arena.h"

#define ZDX_STRING_VIEW_IMPLEMENTATION
#include "./zdx_string_view.h"

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
  sv_t val;
  const char* err;
} json_token_t;

typedef struct {
  size_t cursor;
  size_t line;
  size_t bol; // beginning of line. col = bol - cursor
  sv_t *input;
} json_lexer_t;

const char* json_token_kind_to_cstr(const json_token_kind_t kind)
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

json_token_t json_lexer_next_token(json_lexer_t *const lexer)
{
  json_token_t token = {0};

  // TODO: add assertions for values in lexer struct that make sense here

  if (lexer->input->length == 0 || lexer->cursor >= lexer->input->length) {
    token.kind = JSON_TOKEN_END;
    return token;
  }

  if (isspace(lexer->input->buf[lexer->cursor])) {
    token.kind = JSON_TOKEN_WS;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    if (lexer->input->buf[lexer->cursor] == '\n') {
      lexer->line += 1;
      lexer->bol = lexer->cursor + 1;
    }

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == '{') {
    token.kind = JSON_TOKEN_OCURLY;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == '}') {
    token.kind = JSON_TOKEN_CCURLY;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == '[') {
    token.kind = JSON_TOKEN_OSQR;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == ']') {
    token.kind = JSON_TOKEN_CSQR;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == ':') {
    token.kind = JSON_TOKEN_COLON;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == ',') {
    token.kind = JSON_TOKEN_COMMA;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  // null, true, false
  if (isalpha(lexer->input->buf[lexer->cursor])) {
    token.kind = JSON_TOKEN_SYMBOL;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;

    // ok to check rather than assert cursor < input.length since the already
    // consumed character is a valid token and hence we have something to return
    // even when this loop doesn't run.
    while(lexer->cursor < lexer->input->length && isalpha(lexer->input->buf[lexer->cursor])) {
      token.val.length += 1;
      lexer->cursor += 1;
    }
    return token;
  }

  // Instead of asserts we instead return JSON_TOKEN_UNKNOWN with captured unknown chars in string view so that
  // parser can show useful errors. This assertm() stuff is only during development of the lib to debug logic quickly.
  // We really want to return a value rather than abort as failing at lexing shouldn't fail the application that's
  // using the lexer + parser.
  // numbers
  int curr_char_isdigit = isdigit(lexer->input->buf[lexer->cursor]);
  int curr_char_isplus = lexer->input->buf[lexer->cursor] == '+';
  int curr_char_isminus = lexer->input->buf[lexer->cursor] == '-';

  if (curr_char_isdigit || curr_char_isplus || curr_char_isminus) {
    if (curr_char_isdigit) {
      token.kind = JSON_TOKEN_LONG;
      token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1); // consume <digit>

      lexer->cursor += 1; // move cursor past <digit>
    }
    else if (curr_char_isplus || curr_char_isminus) {
      if ((lexer->cursor + 1) >= lexer->input->length || !isdigit(lexer->input->buf[lexer->cursor + 1])) {
        token.kind = JSON_TOKEN_UNKNOWN;
        token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1); // consume (<plus> | <minus>)
        token.err = "Expected a digit to follow";

        lexer->cursor += 1; // move cursor past (<plus> | <minus>)
        return token;
      }

      /* assertm((lexer->cursor + 1) < lexer->input->length, */
              /* "Reached end when expected digit at line %zu col %zu", lexer->line, (lexer->cursor + 1) - lexer->bol); */
      /* assertm(isdigit(lexer->input->buf[lexer->cursor + 1]), */
              /* "Expected a digit at line %zu col %zu but received '%c'", */
              /* lexer->line, (lexer->cursor + 1) - lexer->bol, lexer->input->buf[lexer->cursor + 1]); */

      token.kind = JSON_TOKEN_LONG;
      token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 2); // consume (<plus> | <minus>)<digit>
                                                                     //
      lexer->cursor += 2; // move cursor past (<plus> | <minus>)<digit>
    }
    else {
      assertm(false, "JSON LEXER: THIS SHOULD BE UNREACHABLE");
    }

    while(lexer->cursor < lexer->input->length && isdigit(lexer->input->buf[lexer->cursor])) {
      token.val.length += 1;
      lexer->cursor += 1;
    }

    // floating point component
    if (lexer->cursor < lexer->input->length && lexer->input->buf[lexer->cursor] == '.') {
      /* assertm((lexer->cursor + 1) < lexer->input->length, */
      /*         "Reached end when expected digit at line %zu col %zu", lexer->line, (lexer->cursor + 1) - lexer->bol); */
      /* assertm(isdigit(lexer->input->buf[lexer->cursor + 1]), */
      /*         "Expected a digit at line %zu col %zu but received '%c'", */
      /*         lexer->line, (lexer->cursor + 1) - lexer->bol, lexer->input->buf[lexer->cursor + 1]); */

      if ((lexer->cursor + 1) >= lexer->input->length || !isdigit(lexer->input->buf[lexer->cursor + 1])) {
        token.kind = JSON_TOKEN_UNKNOWN;
        token.val.length += 1; // consume <dot>
        token.err = "Expected a digit to follow";

        lexer->cursor += 1; // move cursor past <dot>
        return token;
      }

      token.kind = JSON_TOKEN_DOUBLE;
      token.val.length += 2; // consume <dot><digit>
      lexer->cursor += 2; // move cursor past <dot><digit>

      while(lexer->cursor < lexer->input->length && isdigit(lexer->input->buf[lexer->cursor])) {
        token.val.length += 1;
        lexer->cursor += 1;
      }
    }

    // exponent component
    if (lexer->cursor < lexer->input->length && (lexer->input->buf[lexer->cursor] == 'e' || lexer->input->buf[lexer->cursor] == 'E')) {
      /* assertm((lexer->cursor + 1) < lexer->input->length, */
      /*         "Reached end when expected '+', '-' or a digit at line %zu col %zu", lexer->line, (lexer->cursor + 1) - lexer->bol); */

      if ((lexer->cursor + 1) >= lexer->input->length) {
          token.kind = JSON_TOKEN_UNKNOWN;
          token.val.length += 1; // consume (<e> | <E>)
          token.err = "Expected a '+', '-' or a digit to follow";

          lexer->cursor += 1; // move cursor past (<e> | <E>)
          return token;
        }


      int next_char_isdigit = isdigit(lexer->input->buf[lexer->cursor + 1]);
      int next_char_isplus = lexer->input->buf[lexer->cursor + 1] == '+';
      int next_char_isminus = lexer->input->buf[lexer->cursor + 1] == '-';

      if (next_char_isdigit) {
        token.val.length += 2; // consume <e><digit>
        lexer->cursor += 2; // move cursor past <e><digit>
      }
      else if (next_char_isplus || next_char_isminus) {
        /* assertm((lexer->cursor + 2) < lexer->input->length, */
        /*         "Reached end when expected a digit at line %zu col %zu", lexer->line, (lexer->cursor + 2) - lexer->bol); */
        /* assertm(isdigit(lexer->input->buf[lexer->cursor + 2]), */
        /*         "Expected a digit at line %zu col %zu but received '%c'", */
        /*         lexer->line, (lexer->cursor + 2) - lexer->bol, lexer->input->buf[lexer->cursor + 2]); */

        if ((lexer->cursor + 2) >= lexer->input->length || !isdigit(lexer->input->buf[lexer->cursor + 2])) {

          token.kind = JSON_TOKEN_UNKNOWN;
          token.val.length += 2; // consume (<e> | <E>)(<plus> | <minus>)
          token.err = "Expected a digit to follow";

          lexer->cursor += 2; // move cursor past (<e> | <E>)(<plus> | <minus>)
          return token;
        }

        token.val.length += 3; // consume <e>(<plus> | <minus>)<digit>
        lexer->cursor += 3; // move cursor past <e>(<plus> | <minus>)<digit>
      }
      else {
        /* assertm(false, "Invalid scientific notation number at line %zu col %zu", lexer->line, (lexer->cursor + 1) - lexer->bol); */
        token.kind = JSON_TOKEN_UNKNOWN;
        token.val.length += 1;
        token.err = "Expected a '+', '-' or a digit to follow";

        lexer->cursor += 1; // move cursor past (<e> | <E>)
        return token;
      }

      while (lexer->cursor < lexer->input->length && isdigit(lexer->input->buf[lexer->cursor])) {
        token.val.length += 1;
        lexer->cursor += 1;
      }
    }

    return token; // valid LONG or DOUBLE token
  }

  // string
  if (lexer->input->buf[lexer->cursor] == '"') {
    token.kind = JSON_TOKEN_STRING;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1); // consume OPENING <dbl_quote>
    lexer->cursor += 1; // move cursor past OPENING <dbl_quote>

    while(true) {
      // end of input before end of string so return JSON_TOKEN_UNKNOWN
      if (lexer->cursor >= lexer->input->length) {
        token.kind = JSON_TOKEN_UNKNOWN;
        token.err = "Expected a closing quote (\")";
        return token;
      }

      if (lexer->input->buf[lexer->cursor] == '"' && lexer->input->buf[lexer->cursor - 1] != '\\') {
        token.val.length += 1; // consume CLOSING <dbl_quote>
        token.kind = JSON_TOKEN_STRING;

        lexer->cursor += 1; // move cursor past CLOSING <dbl_quote>
        return token;
      }

      token.val.length += 1;
      lexer->cursor += 1;
    }

    assertm(false, "JSON LEXER: THIS SHOULD BE UNREACHABLE");
  }

  token.kind = JSON_TOKEN_UNKNOWN; // this is default but I'm hardcoding just in case I re-order the json_token_type_t enum again
  token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);
  token.err = "Unrecognised token";

  lexer->cursor += 1;
  return token;
}

#endif // ZDX_JSON_IMPLEMENTATION
