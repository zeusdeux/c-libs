#include <stdio.h>
#define ZDX_JSON_IMPLEMENTATION
#include "../zdx_json.h"
#include "../zdx_util.h"

int main(void)
{
  {
    json_t j = json_parse("null", 4);
    assertm(j.type == JSON_NULL, "Expected: value to be %s, Received: %s", json_type_cstr(JSON_NULL), json_type_cstr(j.type));
  }

  return 0;
}
