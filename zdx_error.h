#ifndef ZDX_ERROR_H_
#define ZDX_ERROR_H_

#include <stdbool.h>

#ifdef ZDX_ERROR_IMPLEMENTATION_AUTO
#    define ZDX_STRING_VIEW_IMPLEMENTATION
#    define ZDX_ERROR_IMPLEMENTATION
#endif
#include "zdx_util.h"
#include "zdx_string_view.h"

typedef struct {
  union {
    const sv_t msg;
    const sv_t message;
  };
} err_t;

static const err_t err_none = {0};

err_t err_create(const char* msg);
bool err_exists(const err_t[const static 1]);

// ----------------------------------------------------------------------------------------------------------------

#ifdef ZDX_ERROR_IMPLEMENTATION

#define err_dbg(label, err) dbg("%s Err (msg = sv_t(buf = %p, length = %zu))", (label), (void *)(err).msg.buf, (err).msg.length)

err_t err_create(const char* msg)
{
  err_t res = {
    .msg = sv_from_cstr(msg),
  };

  err_dbg("<<", res);
  return res;
}

bool err_exists(const err_t err[const static 1])
{
  return err != NULL && err->msg.buf != NULL;
}

#endif // ZDX_ERROR_IMPLEMENTATION
#endif // ZDX_ERROR_H_
