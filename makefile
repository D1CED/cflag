DBGFLAGS = -Wall -Wextra -std=c17 -pedantic -Og -fsanitize=address -fno-omit-frame-pointer
RLSFLAGS = -O2
CFLAGS = $(DBGFLAGS)
SRCS = flag.c
OBJS = $(SRCS: .c=.o)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

test: flag_test.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: run_test
run_test: test
	./test
	rm test

.PHONY: clean
clean:
	rm $(OBJS)
