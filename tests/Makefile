CC      ?= gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -Werror -I../include
CXX     ?= c++
SRC     := ../src/mlog.c
TESTS   := test_all.c
COMPILE_DIR := compile

.PHONY: all clean compile-checks compile-c99 compile-c11 compile-cpp compile-multi-tu compile-invalid-configs

all: test_all
	@./test_all

test_all: $(TESTS) $(SRC) ../include/mlog.h
	$(CC) $(CFLAGS) $(SRC) $(TESTS) -o $@

compile-checks: compile-c99 compile-c11 compile-cpp compile-multi-tu compile-invalid-configs

compile-c99:
	$(CC) $(CFLAGS) -c $(COMPILE_DIR)/c99_macro_no_args.c -o c99_macro_no_args.o
	$(CC) $(CFLAGS) -c $(COMPILE_DIR)/c99_macro_control_flow.c -o c99_macro_control_flow.o
	$(CC) $(CFLAGS) -c $(COMPILE_DIR)/disabled_macro_no_eval.c -o disabled_macro_no_eval.o
	$(CC) $(CFLAGS) -c $(COMPILE_DIR)/c99_consumer.c -o c99_consumer.o

compile-c11:
	$(CC) -std=c11 -Wall -Wextra -Wpedantic -Werror -I../include -c $(COMPILE_DIR)/c11_consumer.c -o c11_consumer.o

compile-cpp:
	$(CC) $(CFLAGS) -c $(SRC) -o mlog_c.o
	$(CXX) -Wall -Wextra -Werror -I../include $(COMPILE_DIR)/cpp_consumer.cpp mlog_c.o -o cpp_consumer_test

compile-multi-tu:
	$(CC) $(CFLAGS) -c $(SRC) -o mlog_multi.o
	$(CC) $(CFLAGS) $(COMPILE_DIR)/multi_tu_a.c $(COMPILE_DIR)/multi_tu_b.c $(COMPILE_DIR)/multi_tu_main.c mlog_multi.o -o multi_tu_test

compile-invalid-configs:
	powershell -ExecutionPolicy Bypass -File $(COMPILE_DIR)/check_invalid_config.ps1 -CC "$(CC)" -CFlags "$(CFLAGS)"

clean:
	rm -f test_all *.o cpp_consumer_test multi_tu_test
