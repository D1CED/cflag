DBGFLAGS = -Wall -Wextra -std=c11 -pedantic -Og -fsanitize=address -fno-omit-frame-pointer
RLSFLAGS = -O2
CC_FLAGS = $(DBGFLAGS)

flag.o: flag.c flag.h
	$(CC) $(CC_FLAGS) -o flag.o flag.c

test: flag_test.c flag.h flag.o
	$(CC) $(CC_FLAGS) -o test flag_test.c flag.o

.PHONY: run_test
run_test: test
	./test

.PHONY: clean
clean:
	rm -fv *.o test
