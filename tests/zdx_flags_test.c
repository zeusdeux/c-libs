#include <stdio.h>
#include <stdbool.h>


#define ZDX_FLAGS_IMPLEMENTATION_AUTO
#include "../zdx_flags.h"
#include "../zdx_string_view.h"
#include "../zdx_error.h"
#include "../zdx_util.h"
#include "../zdx_test_utils.h"

typedef struct {
  int argc;
  char *argv[128];
  const err_t parse_result;
  const flag_value_t user;
  const flag_value_t debug;
  const flag_value_t service;
  const flag_value_t profile_id;
} test_input_t;

#define assertm_flag_values_eq(received, expected)                      \
  do {                                                                  \
    assertm(sv_eq_sv((received).err.msg, (expected).err.msg),           \
            "Expected: \""SV_FMT"\", Received: \""SV_FMT"\"",           \
            sv_fmt_args((expected).err.msg),                            \
            sv_fmt_args((received).err.msg));                           \
                                                                        \
    assertm((received).kind == (expected).kind,                         \
            "Expected: %d, Received: %d",                               \
            (expected).kind,                                            \
            (received).kind);                                           \
                                                                        \
    switch((expected.kind)) {                                           \
      case FLAG_TYPE_BOOLEAN: {                                         \
        assertm((received).as.boolean == (expected).as.boolean,         \
                "Expected: %s, Received: %s",                           \
                (expected).as.boolean ? "true" : "false",               \
                (received).as.boolean ? "true" : "false");              \
      } break;                                                          \
      case FLAG_TYPE_STRING: {                                          \
        assertm(sv_eq_sv((received).as.sv, (expected).as.sv),           \
                "Expected: \""SV_FMT"\", Received: \""SV_FMT"\"",       \
                sv_fmt_args((expected).as.sv),                          \
                sv_fmt_args((received).as.sv));                         \
      } break;                                                          \
      case FLAG_TYPE_STRING_ARRAY: {                                    \
        assertm((received).as.svs.length == (expected).as.svs.length,   \
                "Expected: %zu, Received: %zu",                         \
                (expected).as.svs.length,                               \
                (received).as.svs.length);                              \
                                                                        \
        for (size_t i = 0; i < (received).as.svs.length; i++) {         \
          sv_t received_item = (received).as.svs.items[i];              \
          sv_t expected_item = (expected).as.svs.items[i];              \
                                                                        \
          assertm(sv_eq_sv(received_item, expected_item),               \
                  "Expected: \""SV_FMT"\", Received: \""SV_FMT"\"",     \
                  sv_fmt_args(expected_item),                           \
                  sv_fmt_args(received_item));                          \
        }                                                               \
      } break;                                                          \
      default: {                                                        \
        assertm(false, "Unknown result kind %d", (expected).kind);      \
      }                                                                 \
    }                                                                   \
  } while(0)

void test(test_input_t input)
{
  for (int i = 0; i < input.argc; i++) {
    testlog(L_INFO, ">> argv[%d] = %s", i, input.argv[i]);
  }

  flag_option_t opt_user = {
    .name = "user",
    .alias = "u",
    // .type = FLAG_TYPE_STRING <- default option type is FLAG_TYPE_STRING
  };

  flag_option_t opt_profile_id = {
    .name = "profile-id",
    .alias = "p",
    .type = FLAG_TYPE_STRING,
  };


  flag_option_t opt_debug = {
    .name = "debug",
    .alias = "d",
    .type = FLAG_TYPE_BOOLEAN,
  };

  flag_option_t opt_service = {
    .name = "service",
    .alias = "s",
    .type = FLAG_TYPE_STRING_ARRAY,
  };

  flags_t flags = {0};

  err_t err = flags_parse(&flags, input.argc, input.argv);

  assertm(sv_eq_sv(err.msg, input.parse_result.msg),
          "Expected: \""SV_FMT"\", Received: \""SV_FMT"\"",
          sv_fmt_args(input.parse_result.msg), sv_fmt_args(err.msg));

  flag_value_t user = flags_get(&flags, opt_user);
  flag_value_t debug = flags_get(&flags, opt_debug);
  flag_value_t service = flags_get(&flags, opt_service);
  flag_value_t profile_id = flags_get(&flags, opt_profile_id);

  assertm_flag_values_eq(user, input.user);
  assertm_flag_values_eq(debug, input.debug);
  assertm_flag_values_eq(service, input.service);
  assertm_flag_values_eq(profile_id, input.profile_id);

  flag_value_deinit(&service); // deinit the FLAG_TYPE_STRING_ARRAY as it's returned as a dynamic array
  flag_value_deinit(&user); // this is a noop as user if FLAG_TYPE_STRING and thus passed by value
  flags_deinit(&flags); // flags (parse result) is a dynamic array and hence needs to be deinit

  testlog(L_INFO, "------------------------------------------------------------");
}

int main(void)
{
  const flag_value_t opt_not_found = {
    .err = err_create("Flag not found"),
  };
  const flag_value_t opt_bool_default = {
    .kind = FLAG_TYPE_BOOLEAN,
    .as = { .boolean = false }
  };
  const flag_value_t opt_string_default = {
    .kind = FLAG_TYPE_STRING,
    .as = { .sv = sv_from_cstr("") },
  };

  const sv_t svc1 = { .buf = "svc1", .length = 4};
  const sv_t svc2 = { .buf = "svc2", .length = 4};
  const sv_t svcs[2] = {svc1, svc2};

  test_input_t inputs[] = {
    // Parse errors
    {
      .argv = {"prg-name"}, .argc = 1,
      .parse_result = err_create("Too few arguments. Expected greated than 2"),
      .user = opt_not_found,
      .debug = opt_bool_default,
      .service = opt_not_found,
      .profile_id = opt_not_found,
    },
    {
      .argv = { [0] = "prg-name", [1] = "value"}, .argc = 2,
      .parse_result = err_create("No flag provided for value"),
      .user = opt_not_found,
      .debug = opt_bool_default,
      .service = opt_not_found,
      .profile_id = opt_not_found,
    },
    {
      .argv = {"prg-name", "some-profile-id"}, .argc = 2,
      .parse_result = err_create("No flag provided for value"),
      .user = opt_not_found,
      .debug = opt_bool_default,
      .service = opt_not_found,
      .profile_id = opt_not_found,
    },

    // FLAG_TYPE_STRING defaults -> sv("")
    {
      .argv = {"prg-name", "--user"}, .argc = 2,
      .parse_result = err_none,
      .user = opt_string_default,
      .debug = opt_bool_default,
      .service = opt_not_found,
      .profile_id = opt_not_found,
    },
    {
      .argv = {"prg-name", "--profile-id"}, .argc = 2,
      .parse_result = err_none,
      .user = opt_not_found,
      .debug = opt_bool_default,
      .service = opt_not_found,
      .profile_id = opt_string_default,
    },

    // FLAG_TYPE_STRING parsed
    {
      .argv = {"prg-name", "-u", "some-user"}, .argc = 3,
      .parse_result = err_none,
      .user = {
        .kind = FLAG_TYPE_STRING,
        .as = { .sv = sv_from_cstr("some-user") }
      },
      .debug = opt_bool_default,
      .service = opt_not_found,
      .profile_id = opt_not_found,
    },
    {
      .argv = {"prg-name", "--user", "some-user"}, .argc = 3,
      .parse_result = err_none,
      .user = {
        .kind = FLAG_TYPE_STRING,
        .as = { .sv = sv_from_cstr("some-user") }
      },
      .debug = opt_bool_default,
      .service = opt_not_found,
      .profile_id = opt_not_found,
    },
    {
      .argv = {"prg-name", "--profile-id", "1234"}, .argc = 3,
      .parse_result = err_none,
      .user = opt_not_found,
      .debug = opt_bool_default,
      .service = opt_not_found,
      .profile_id = {
        .kind = FLAG_TYPE_STRING,
        .as = { .sv = sv_from_cstr("1234") }
      },
    },

    // FLAG_TYPE_BOOLEAN == TRUE
    {
      .argv = {"prg-name", "--debug"}, .argc = 2,
      .parse_result = err_none,
      .user = opt_not_found,
      .debug = {
        .kind = FLAG_TYPE_BOOLEAN,
        .as = {
          .boolean = true
        }
      },
      .service = opt_not_found,
      .profile_id = opt_not_found,
    },
    {
      .argv = {"prg-name", "--debug", "some-user"}, .argc = 3,
      .parse_result = err_none,
      .user = opt_not_found,
      .debug = {
        .kind = FLAG_TYPE_BOOLEAN,
        .as = {
          .boolean = true
        }
      },
      .service = opt_not_found,
      .profile_id = opt_not_found,
    },
    {
      .argv = {"prg-name", "-u", "some-user", "--debug"}, .argc = 4,
      .parse_result = err_none,
      .user = {
        .kind = FLAG_TYPE_STRING,
        .as = {
          .sv = sv_from_cstr("some-user")
        }
      },
      .debug = {
        .kind = FLAG_TYPE_BOOLEAN,
        .as = {
          .boolean = true
        }
      },
      .service = opt_not_found,
      .profile_id = opt_not_found,
    },
    {
      .argv = {"prg-name", "--debug", "-user", "some-user"}, .argc = 4,
      .parse_result = err_none,
      .user = {
        .kind = FLAG_TYPE_STRING,
        .as = {
          .sv = sv_from_cstr("some-user")
        }
      },
      .debug = {
        .kind = FLAG_TYPE_BOOLEAN,
        .as = {
          .boolean = true
        }
      },
      .service = opt_not_found,
      .profile_id = opt_not_found,
    },
    {
      .argv = {"prg-name", "--debug", "--user", "some-user"}, .argc = 4,
      .parse_result = err_none,
      .user = {
        .kind = FLAG_TYPE_STRING,
        .as = {
          .sv = sv_from_cstr("some-user")
        }
      },
      .debug = {
        .kind = FLAG_TYPE_BOOLEAN,
        .as = {
          .boolean = true
        }
      },
      .service = opt_not_found,
      .profile_id = opt_not_found,
    },
    // FLAG_TYPE_BOOLEAN == FALSE
    {
      .argv = {"prg-name", "--user", "some-user"}, .argc = 3,
      .parse_result = err_none,
      .user = {
        .kind = FLAG_TYPE_STRING,
        .as = {
          .sv = sv_from_cstr("some-user")
        }
      },
      .debug = opt_bool_default,
      .service = opt_not_found,
      .profile_id = opt_not_found,
    },

    // FLAG_TYPE_STRING_ARRAY
    {
      .argv = {"prg-name", "--service", "svc1", "-service", "svc2"}, .argc = 5,
      .parse_result = err_none,
      .user = opt_not_found,
      .debug = opt_bool_default,
      .service = {
        .kind = FLAG_TYPE_STRING_ARRAY,
        .as = {
          .svs = {
            .length = 2,
            .capacity = 8,
            .items = (sv_t *)svcs
          }
        }
      },
      .profile_id = opt_not_found,
    },
    {
      .argv = {"prg-name", "--service", "svc1", "-s", "svc2"}, .argc = 5,
      .parse_result = err_none,
      .user = opt_not_found,
      .debug = opt_bool_default,
      .service = {
        .kind = FLAG_TYPE_STRING_ARRAY,
        .as = {
          .svs = {
            .length = 2,
            .capacity = 8,
            .items = (sv_t *)svcs
          }
        }
      },
      .profile_id = opt_not_found,
    },
    {
      .argv = {"prg-name", "-s", "svc1", "-service", "svc2"}, .argc = 5,
      .parse_result = err_none,
      .user = opt_not_found,
      .debug = opt_bool_default,
      .service = {
        .kind = FLAG_TYPE_STRING_ARRAY,
        .as = {
          .svs = {
            .length = 2,
            .capacity = 8,
            .items = (sv_t *)svcs
          }
        }
      },
      .profile_id = opt_not_found,
    },
    {
      .argv = {"prg-name", "-service", "svc1", "-s", "svc2"}, .argc = 5,
      .parse_result = err_none,
      .user = opt_not_found,
      .debug = opt_bool_default,
      .service = {
        .kind = FLAG_TYPE_STRING_ARRAY,
        .as = {
          .svs = {
            .length = 2,
            .capacity = 8,
            .items = (sv_t *)svcs
          }
        }
      },
      .profile_id = opt_not_found,
    },
    {
      .argv = {"prg-name", "-s", "svc1", "-s", "svc2"}, .argc = 5,
      .parse_result = err_none,
      .user = opt_not_found,
      .debug = opt_bool_default,
      .service = {
        .kind = FLAG_TYPE_STRING_ARRAY,
        .as = {
          .svs = {
            .length = 2,
            .capacity = 8,
            .items = (sv_t *)svcs
          }
        }
      },
      .profile_id = opt_not_found,
    },
    {
      .argv = {"prg-name", "-s", "svc1", "-d", "bruh" }, .argc = 5,
      .parse_result = err_none,
      .user = opt_not_found,
      .debug = {
        .kind = FLAG_TYPE_BOOLEAN,
        .as = {
          .boolean = true
        }
      },
      .service = {
        .kind = FLAG_TYPE_STRING_ARRAY,
        .as = {
          .svs = {
            // LENGTH = 1 IS IMPORTANT!! It lets us pretend that .items is
            // an array of length one hence only containing svc1 as that's
            // what we parse in this test case "-s svc1"
            .length = 1,
            .capacity = 2,
            .items = (sv_t *)svcs
          }
        }
      },
      .profile_id = opt_not_found,
    },

  };

  for (size_t i = 0; i < zdx_arr_len(inputs); i++) {
    test(inputs[i]);
  }

  testlog(L_INFO, "<zdx_flags_test> All ok!\n");

  return 0;
}
