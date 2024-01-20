FLAGS = -std=c17 -Wall -pedantic -O3
TEST_FLAGS = $(FLAGS) -Wno-gnu-statement-expression-from-macro-expansion -Wno-macro-redefined

DBG_FLAGS = -DZDX_TRACE_ENABLE -std=c17 -pedantic -g -Wall -Wextra -Wdeprecated
DBG_TEST_FLAGS = $(DBG_FLAGS) -Wno-gnu-statement-expression-from-macro-expansion -Wno-macro-redefined


test_da:
	@clang $(TEST_FLAGS) ./tests/zdx_da_test.c -o ./tests/zdx_da_test && leaks --atExit -- ./tests/zdx_da_test

test_da_dbg:
	@clang $(DBG_TEST_FLAGS) ./tests/zdx_da_test.c -o ./tests/zdx_da_test && leaks --atExit -- ./tests/zdx_da_test

clean:
	$(RM) -fr ./tests/*_test ./tests/*.dSYM

.PHONY: clean test_*
