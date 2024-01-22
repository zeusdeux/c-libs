FLAGS = -std=c17 -Wall -pedantic -O3
TEST_FLAGS = $(FLAGS)

DBG_FLAGS = -DZDX_TRACE_ENABLE -std=c17 -pedantic -g -Wall -Wextra -Wdeprecated
DBG_TEST_FLAGS = $(DBG_FLAGS)


test_zdx_da:
	@clang $(TEST_FLAGS) ./tests/zdx_da_test.c -o ./tests/zdx_da_test && ./tests/zdx_da_test
	@echo "--- Checking for memory leaks ---"
	@ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet --atExit -- ./tests/zdx_da_test

test_zdx_da_dbg:
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_da_test.c -o ./tests/zdx_da_test_dbg && ./tests/zdx_da_test_dbg
	@echo "--- Checking for memory leaks ---"
	@ZDX_DISABLE_TEST_OUTPUT=true leaks --atExit -- ./tests/zdx_da_test_dbg

test_zdx_str:
	@clang $(TEST_FLAGS) ./tests/zdx_str_test.c -o ./tests/zdx_str_test && ./tests/zdx_str_test
	@echo "--- Checking for memory leaks ---"
	@ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet --atExit -- ./tests/zdx_str_test

test_zdx_str_dbg:
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_str_test.c -o ./tests/zdx_str_test_dbg && ./tests/zdx_str_test_dbg
	@echo "--- Checking for memory leaks ---"
	@ZDX_DISABLE_TEST_OUTPUT=true leaks --atExit -- ./tests/zdx_str_test_dbg

test: test_zdx_da test_zdx_str

clean:
	$(RM) -fr ./tests/*_test ./tests/*_test_dbg ./tests/*.memgraph ./tests/*.dSYM

.PHONY: clean test_* test
