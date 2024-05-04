DBG_LINKER_FLAGS = -Wl,-v # show details of linker invocation by clang
FLAGS = -std=c17 -Wall -pedantic -O3
TEST_FLAGS = $(FLAGS)

DBG_FLAGS = -DZDX_TRACE_ENABLE -DDEBUG -std=c17 -pedantic -g -Wall -Wextra -Wdeprecated -fsanitize=address,undefined
DBG_TEST_FLAGS = $(DBG_FLAGS) $(LINKER_FLAGS)

tags:
	@etags /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/**/*.h ./*.h ./**/*.c

test_zdx_da:
	@echo "--- Running tests on zdx_da.h release ---"
	@clang $(TEST_FLAGS) ./tests/zdx_da_test.c -o ./tests/zdx_da_test && ./tests/zdx_da_test
	@echo "--- Checking for memory leaks in zdx_da.h ---"
	@ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet 2>/dev/null --atExit -- ./tests/zdx_da_test

test_zdx_da_dbg:
	@echo "--- Running tests on zdx_da.h debug ---"
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_da_test.c -o ./tests/zdx_da_test_dbg && ./tests/zdx_da_test_dbg
	@echo "--- Checking for memory leaks in zdx_da.h ---"
	leaks --atExit -- ./tests/zdx_da_test_dbg

test_zdx_str:
	@echo "--- Running tests on zdx_str.h release ---"
	@clang $(TEST_FLAGS) ./tests/zdx_str_test.c -o ./tests/zdx_str_test && ./tests/zdx_str_test
	@echo "--- Checking for memory leaks in zdx_str.h ---"
	@ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet 2>/dev/null --atExit -- ./tests/zdx_str_test

test_zdx_str_dbg:
	@echo "--- Running tests on zdx_str.h debug ---"
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_str_test.c -o ./tests/zdx_str_test_dbg && ./tests/zdx_str_test_dbg
	@echo "--- Checking for memory leaks in zdx_str.h ---"
	leaks --atExit -- ./tests/zdx_str_test_dbg

test_zdx_gap_buffer:
	@echo "--- Running tests on zdx_gap_buffer.h release ---"
	@clang $(TEST_FLAGS) ./tests/zdx_gap_buffer_test.c -o ./tests/zdx_gap_buffer_test && ./tests/zdx_gap_buffer_test
	@echo "--- Checking for memory leaks in zdx_gap_buffer.h ---"
	@ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet 2>/dev/null --atExit -- ./tests/zdx_gap_buffer_test

test_zdx_gap_buffer_dbg:
	@echo "--- Running tests on zdx_gap_buffer.h debug ---"
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_gap_buffer_test.c -o ./tests/zdx_gap_buffer_test_dbg && ./tests/zdx_gap_buffer_test_dbg
	@echo "--- Checking for memory leaks in zdx_gap_buffer.h ---"
	leaks --atExit -- ./tests/zdx_gap_buffer_test_dbg

test_zdx_string_view:
	@echo "--- Running tests on zdx_string_view.h release ---"
	@clang $(TEST_FLAGS) ./tests/zdx_string_view_test.c -o ./tests/zdx_string_view_test && ./tests/zdx_string_view_test
	@echo "--- Checking for memory leaks in zdx_string_view.h ---"
	@ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet --atExit 2>/dev/null -- ./tests/zdx_string_view_test

test_zdx_string_view_dbg:
	@echo "--- Running tests on zdx_string_view.h debug ---"
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_string_view_test.c -o ./tests/zdx_string_view_test_dbg && ./tests/zdx_string_view_test_dbg
	@echo "--- Checking for memory leaks in zdx_string_view.h ---"
	leaks --atExit -- ./tests/zdx_string_view_test_dbg

test_zdx_simple_arena:
	@echo "--- Running tests on zdx_simple_arena.h release including DEBUG flow ---"
	@clang -DDEBUG $(TEST_FLAGS) ./tests/zdx_simple_arena_test.c -o ./tests/zdx_simple_arena_test && ./tests/zdx_simple_arena_test
	@echo "--- Checking for memory leaks in zdx_simple_arena.h ---"
	@ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet --atExit 2>/dev/null -- ./tests/zdx_simple_arena_test

test_zdx_simple_arena_dbg:
	@echo "--- Running tests on zdx_simple_arena.h debug ---"
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_simple_arena_test.c -o ./tests/zdx_simple_arena_test_dbg && ./tests/zdx_simple_arena_test_dbg
	@echo "--- Checking for memory leaks in zdx_simple_arena.h ---"
	leaks --atExit -- ./tests/zdx_simple_arena_test_dbg

test_zdx_hashtable:
	@echo "--- Running tests on zdx_hashtable.h release ---"

# using arena_t from zdx_simple_arena.h. Also no free needed as we are using an arena
	@clang -DHT_ARENA_TYPE=arena_t \
		-DHT_CALLOC=arena_calloc -D'HT_FREE(...)' \
		-DTEST_PROLOGUE="testlog(L_INFO, \"<zdx_hashtable_arena_test> Starting tests...\")" \
		-DTEST_EPILOGUE="testlog(L_INFO, \"<zdx_hashtable_arena_test> All ok!\n\")" \
		$(TEST_FLAGS) ./tests/zdx_hashtable_test.c -o ./tests/zdx_hashtable_with_arena_test && ./tests/zdx_hashtable_with_arena_test

	@clang \
		-DTEST_PROLOGUE="testlog(L_INFO, \"<zdx_hashtable_non_arena_test> Starting tests...\")" \
		-DTEST_EPILOGUE="testlog(L_INFO, \"<zdx_hashtable_non_arena_test> All ok!\n\")" \
		$(TEST_FLAGS) ./tests/zdx_hashtable_test.c -o ./tests/zdx_hashtable_without_arena_test && ./tests/zdx_hashtable_without_arena_test

	@echo "--- Checking for memory leaks in zdx_hashtable.h with an arena allocator ---"
	@ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet --atExit 2>/dev/null -- ./tests/zdx_hashtable_with_arena_test

	@echo "--- Checking for memory leaks in zdx_hashtable.h with calloc(3) ---"
	@ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet --atExit 2>/dev/null -- ./tests/zdx_hashtable_without_arena_test

test_zdx_hashtable_dbg:
	@echo "--- Running tests on zdx_hashtable.h debug ---"

# using arena_t from zdx_simple_arena.h. Also no free needed as we are using an arena
	@clang -DHT_ARENA_TYPE=arena_t \
		-DHT_CALLOC=arena_calloc -D'HT_FREE(...)' \
		-DTEST_PROLOGUE="testlog(L_INFO, \"<zdx_hashtable_arena_test> Starting tests...\")" \
		-DTEST_EPILOGUE="testlog(L_INFO, \"<zdx_hashtable_arena_test> All ok!\n\")" \
		$(DBG_TEST_FLAGS) ./tests/zdx_hashtable_test.c -o ./tests/zdx_hashtable_with_arena_test_dbg && ./tests/zdx_hashtable_with_arena_test_dbg

	@clang \
		-DTEST_PROLOGUE="testlog(L_INFO, \"<zdx_hashtable_non_arena_test> Starting tests...\")" \
		-DTEST_EPILOGUE="testlog(L_INFO, \"<zdx_hashtable_non_arena_test> All ok!\n\")" \
		$(DBG_TEST_FLAGS) ./tests/zdx_hashtable_test.c -o ./tests/zdx_hashtable_without_arena_test_dbg && ./tests/zdx_hashtable_without_arena_test_dbg

	@echo "--- Checking for memory leaks in zdx_hashtable.h with an arena allocator ---"
	leaks --quiet --atExit -- ./tests/zdx_hashtable_with_arena_test

	@echo "--- Checking for memory leaks in zdx_hashtable.h with calloc(3) ---"
	leaks --quiet --atExit -- ./tests/zdx_hashtable_without_arena_test


test: test_zdx_da test_zdx_str test_zdx_gap_buffer test_zdx_string_view test_zdx_simple_arena test_zdx_hashtable

test_dbg: test_zdx_da_dbg test_zdx_str_dbg test_zdx_gap_buffer_dbg test_zdx_string_view_dbg test_zdx_simple_arena_dbg test_zdx_hashtable_dbg

clean:
	$(RM) -fr ./tests/*_test ./tests/*_test_dbg ./tests/*.memgraph ./*.dSYM ./tests/*.dSYM

.PHONY: clean test_* test tags
