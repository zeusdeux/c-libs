FLAGS = -std=c17 -Wall -Werror=nonnull -Werror=null-dereference -pedantic -O3
TEST_FLAGS = $(FLAGS) -Wno-gnu-statement-expression-from-macro-expansion -Wno-macro-redefined

DBG_FLAGS = -DZDX_TRACE_ENABLE -std=c17 -pedantic -g -Wall -Wextra -Wdeprecated -Werror=nonnull -Werror=null-dereference
DBG_TEST_FLAGS = $(DBG_FLAGS) -Wno-gnu-statement-expression-from-macro-expansion -Wno-macro-redefined


test_zdx_da:
	@clang $(TEST_FLAGS) ./tests/zdx_da_test.c -o ./tests/zdx_da_test && ./tests/zdx_da_test
	@echo "--- Checking for memory leaks ---"
	@ZDX_DISABLE_TEST_OUTPUT=true leaks --quiet --atExit -- ./tests/zdx_da_test

test_zdx_da_dbg:
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_da_test.c -o ./tests/zdx_da_test_dbg && ./tests/zdx_da_test_dbg
	@echo "--- Checking for memory leaks ---"
	@ZDX_DISABLE_TEST_OUTPUT=true leaks --atExit -- ./tests/zdx_da_test_dbg

clean:
	$(RM) -fr ./tests/*_test ./tests/*_test_dbg ./tests/*.memgraph ./tests/*.dSYM

.PHONY: clean test_*
