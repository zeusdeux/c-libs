FLAGS = -std=c17 -Wall -Wextra -Wpedantic -Wdeprecated -Werror -pedantic -g
TEST_FLAGS = $(FLAGS) -O3

DBG_FLAGS = -DZDX_TRACE_ENABLE -DDEBUG -std=c17 -pedantic -g -Wall -Wextra -Wdeprecated -Werror # -fsanitize=address,undefined <-- doesn't work on macos due to system integrity protection and actually breaks the leaks tool
DBG_TEST_FLAGS = $(DBG_FLAGS)
DBG_LINKER_FLAGS = -Wl,-v # show details of linker invocation by clang

BENCHMARK_FLAGS = $(FLAGS) -O2 -DZDX_LOGS_DISABLE -DNDEBUG -DZDX_PROF_ENABLE

tags:
	@etags /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/**/*.h ./*.h ./**/*.c
	@find -E `pwd` -type f -regex ".+\.(c|h)$ " > cscope.files # the <space> after "$" in the regex is because make throws an error saying missing double quote `"` after `$` otherwise. Idk why
	@cscope -b -q

test_zdx_util:
	@echo "--- Running tests on zdx_util.h release ---"
	@clang $(TEST_FLAGS) ./tests/zdx_util_test.c -o ./tests/zdx_util_test && ./tests/zdx_util_test
	@echo "--- Checking for memory leaks in zdx_util.h ---"
	@if [ -z "${CI}" ]; then ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet 2>/dev/null --atExit -- ./tests/zdx_util_test; else :; fi

test_zdx_util_dbg:
	@echo "--- Running tests on zdx_util.h debug ---"
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_util_test.c -o ./tests/zdx_util_test_dbg && ./tests/zdx_util_test_dbg
	@echo "--- Checking for memory leaks in zdx_util.h ---"
	@if [ -z "${CI}" ]; then leaks --atExit -- ./tests/zdx_util_test_dbg; else :; fi

test_zdx_da:
	@echo "--- Running tests on zdx_da.h release ---"
	@clang $(TEST_FLAGS) ./tests/zdx_da_test.c -o ./tests/zdx_da_test && ./tests/zdx_da_test
	@echo "--- Checking for memory leaks in zdx_da.h ---"
	@if [ -z "${CI}" ]; then ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet 2>/dev/null --atExit -- ./tests/zdx_da_test; else :; fi

test_zdx_da_dbg:
	@echo "--- Running tests on zdx_da.h debug ---"
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_da_test.c -o ./tests/zdx_da_test_dbg && ./tests/zdx_da_test_dbg
	@echo "--- Checking for memory leaks in zdx_da.h ---"
	@if [ -z "${CI}" ]; then leaks --atExit -- ./tests/zdx_da_test_dbg; else :; fi

test_zdx_str:
	@echo "--- Running tests on zdx_str.h release ---"
	@clang $(TEST_FLAGS) ./tests/zdx_str_test.c -o ./tests/zdx_str_test && ./tests/zdx_str_test
	@echo "--- Checking for memory leaks in zdx_str.h ---"
	@if [ -z "${CI}" ]; then ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet 2>/dev/null --atExit -- ./tests/zdx_str_test; else :; fi

test_zdx_str_dbg:
	@echo "--- Running tests on zdx_str.h debug ---"
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_str_test.c -o ./tests/zdx_str_test_dbg && ./tests/zdx_str_test_dbg
	@echo "--- Checking for memory leaks in zdx_str.h ---"
	@if [ -z "${CI}" ]; then leaks --atExit -- ./tests/zdx_str_test_dbg; else :; fi

test_zdx_gap_buffer:
	@echo "--- Running tests on zdx_gap_buffer.h release ---"
        # enabling the _POSIX_C_SOURCE macro + including stdio seems to be necessary on ubuntu as per docs
        # to get access to functions such as fileno(3)
        #    https://manpages.ubuntu.com/manpages/lunar/man3/fileno.3.html
        # why ubuntu? well, that's what the runner for github actions uses
	@clang -D_GNU_SOURCE $(TEST_FLAGS) ./tests/zdx_gap_buffer_test.c -o ./tests/zdx_gap_buffer_test && ./tests/zdx_gap_buffer_test
	@echo "--- Checking for memory leaks in zdx_gap_buffer.h ---"
	@if [ -z "${CI}" ]; then ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet 2>/dev/null --atExit -- ./tests/zdx_gap_buffer_test; else :; fi

test_zdx_gap_buffer_dbg:
	@echo "--- Running tests on zdx_gap_buffer.h debug ---"
        # enabling the _GNU_SOURCE macro + including stdio seems to be necessary on ubuntu as per docs
        # to get access to functions such as fileno(3)
        #    https://manpages.ubuntu.com/manpages/lunar/man3/fileno.3.html
        # why ubuntu? well, that's what the runner for github actions uses
	@clang -D_GNU_SOURCE $(DBG_TEST_FLAGS) ./tests/zdx_gap_buffer_test.c -o ./tests/zdx_gap_buffer_test_dbg && ./tests/zdx_gap_buffer_test_dbg
	@echo "--- Checking for memory leaks in zdx_gap_buffer.h ---"
	@if [ -z "${CI}" ]; then leaks --atExit -- ./tests/zdx_gap_buffer_test_dbg; else :; fi

test_zdx_string_view:
	@echo "--- Running tests on zdx_string_view.h release ---"
	@clang $(TEST_FLAGS) ./tests/zdx_string_view_test.c -o ./tests/zdx_string_view_test && ./tests/zdx_string_view_test
	@echo "--- Checking for memory leaks in zdx_string_view.h ---"
	@if [ -z "${CI}" ]; then ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet --atExit 2>/dev/null -- ./tests/zdx_string_view_test; else :; fi

test_zdx_string_view_dbg:
	@echo "--- Running tests on zdx_string_view.h debug ---"
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_string_view_test.c -o ./tests/zdx_string_view_test_dbg && ./tests/zdx_string_view_test_dbg
	@echo "--- Checking for memory leaks in zdx_string_view.h ---"
	@if [ -z "${CI}" ]; then leaks --atExit -- ./tests/zdx_string_view_test_dbg; else :; fi

test_zdx_simple_arena:
	@echo "--- Running tests on zdx_simple_arena.h release including debug flow ---"
	@clang $(TEST_FLAGS) ./tests/zdx_simple_arena_test.c -o ./tests/zdx_simple_arena_test && ./tests/zdx_simple_arena_test
	@echo "--- Checking for memory leaks in zdx_simple_arena.h ---"
	@if [ -z "${CI}" ]; then ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet --atExit 2>/dev/null -- ./tests/zdx_simple_arena_test; else :; fi

test_zdx_simple_arena_dbg:
	@echo "--- Running tests on zdx_simple_arena.h debug ---"
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_simple_arena_test.c -o ./tests/zdx_simple_arena_test_dbg && ./tests/zdx_simple_arena_test_dbg
	@echo "--- Checking for memory leaks in zdx_simple_arena.h ---"
	@if [ -z "${CI}" ]; then leaks --atExit -- ./tests/zdx_simple_arena_test_dbg; else :; fi

test_zdx_hashtable:
	@echo "--- Running tests on zdx_hashtable.h with arena allocator for release ---"
# using arena_t from zdx_simple_arena.h. Also no free needed as we are using an arena
	@clang -DHT_ARENA_TYPE=arena_t \
		-DHT_CALLOC=arena_calloc -D'HT_FREE(...)' \
		-DTEST_PROLOGUE="testlog(L_INFO, \"<zdx_hashtable_arena_test> Starting tests...\")" \
		-DTEST_EPILOGUE="testlog(L_INFO, \"<zdx_hashtable_arena_test> All ok!\n\")" \
		$(TEST_FLAGS) ./tests/zdx_hashtable_test.c -o ./tests/zdx_hashtable_with_arena_test && ./tests/zdx_hashtable_with_arena_test

	@echo "--- Running tests on zdx_hashtable.h with calloc(3) for release ---"
	@clang \
		-DTEST_PROLOGUE="testlog(L_INFO, \"<zdx_hashtable_non_arena_test> Starting tests...\")" \
		-DTEST_EPILOGUE="testlog(L_INFO, \"<zdx_hashtable_non_arena_test> All ok!\n\")" \
		$(TEST_FLAGS) ./tests/zdx_hashtable_test.c -o ./tests/zdx_hashtable_without_arena_test && ./tests/zdx_hashtable_without_arena_test

	@echo "--- Checking for memory leaks in zdx_hashtable.h with an arena allocator ---"
	@if [ -z "${CI}" ]; then ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet --atExit 2>/dev/null -- ./tests/zdx_hashtable_with_arena_test; else :; fi

	@echo "--- Checking for memory leaks in zdx_hashtable.h with calloc(3) ---"
	@if [ -z "${CI}" ]; then ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet --atExit 2>/dev/null -- ./tests/zdx_hashtable_without_arena_test; else :; fi

test_zdx_hashtable_dbg:
	@echo "--- Running tests on zdx_hashtable.h with arena allocator for debug ---"
# using arena_t from zdx_simple_arena.h. Also no free needed as we are using an arena
	@clang -DHT_ARENA_TYPE=arena_t \
		-DHT_CALLOC=arena_calloc -D'HT_FREE(...)' \
		-DTEST_PROLOGUE="testlog(L_INFO, \"<zdx_hashtable_arena_test> Starting tests...\")" \
		-DTEST_EPILOGUE="testlog(L_INFO, \"<zdx_hashtable_arena_test> All ok!\n\")" \
		$(DBG_TEST_FLAGS) ./tests/zdx_hashtable_test.c -o ./tests/zdx_hashtable_with_arena_test_dbg && ./tests/zdx_hashtable_with_arena_test_dbg

	@echo "--- Running tests on zdx_hashtable.h with calloc(3) for debug ---"
	@clang \
		-DTEST_PROLOGUE="testlog(L_INFO, \"<zdx_hashtable_non_arena_test> Starting tests...\")" \
		-DTEST_EPILOGUE="testlog(L_INFO, \"<zdx_hashtable_non_arena_test> All ok!\n\")" \
		$(DBG_TEST_FLAGS) ./tests/zdx_hashtable_test.c -o ./tests/zdx_hashtable_without_arena_test_dbg && ./tests/zdx_hashtable_without_arena_test_dbg

	@echo "--- Checking for memory leaks in zdx_hashtable.h with an arena allocator ---"
	@if [ -z "${CI}" ]; then leaks --quiet --atExit -- ./tests/zdx_hashtable_with_arena_test; else :; fi

	@echo "--- Checking for memory leaks in zdx_hashtable.h with calloc(3) ---"
	@if [ -z "${CI}" ]; then leaks --quiet --atExit -- ./tests/zdx_hashtable_without_arena_test; else :; fi

test_zdx_flags:
	@echo "--- Running tests on zdx_flags.h release ---"
	@clang $(TEST_FLAGS) ./tests/zdx_flags_test.c -o ./tests/zdx_flags_test && ./tests/zdx_flags_test
	@echo "--- Checking for memory leaks in zdx_flags.h ---"
	@if [ -z "${CI}" ]; then ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet --atExit 2>/dev/null -- ./tests/zdx_flags_test; else :; fi

test_zdx_flags_dbg:
	@echo "--- Running tests on zdx_flags.h debug ---"
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_flags_test.c -o ./tests/zdx_flags_test_dbg && ./tests/zdx_flags_test_dbg
	@echo "--- Checking for memory leaks in zdx_flags.h ---"
	@if [ -z "${CI}" ]; then leaks --atExit -- ./tests/zdx_flags_test_dbg; else :; fi

benchmark_zdx_fast_hashtable:
	@echo "--- Benchmarking zdx_fast_hashtable.h ---"
	@clang $(BENCHMARK_FLAGS) ./benchmarks/zdx_fast_hashtable_benchmark.c -o ./benchmarks/zdx_fast_hashtable_benchmark && ./benchmarks/zdx_fast_hashtable_benchmark


benchmark: benchmark_zdx_fast_hashtable

bench: benchmark

test: test_zdx_util test_zdx_da test_zdx_str test_zdx_gap_buffer test_zdx_string_view test_zdx_simple_arena test_zdx_hashtable test_zdx_flags

test_dbg: test_zdx_util_dbg test_zdx_da_dbg test_zdx_str_dbg test_zdx_gap_buffer_dbg test_zdx_string_view_dbg test_zdx_simple_arena_dbg test_zdx_hashtable_dbg test_zdx_flags_dbg

clean:
	$(RM) -fr ./tests/*_test ./tests/*_test_dbg ./tests/*.memgraph ./*.dSYM ./tests/*.dSYM

.PHONY: clean test_* test bench benchmark tags
