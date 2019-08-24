#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "flag.h"

static void parse_example()
{
	struct Flagset *flags = malloc(FLAGSET_SIZE(5));
	flag_init(flags, 5, "This is a test programm", NULL);

	bool b = false;
	int i = 0;
	long long l = 1L << 33;
	char port[6] = ":80";

	flag_varbool  (flags, &b, "b", "`bool` test");
	flag_varint   (flags, &i, "int", "an `int` test");
	flag_varlong  (flags, &l, "ll", "hoho");
	flag_varstring(flags, port, sizeof(port)-1, "port", "specifiy a port");

	const char *argv[] = {
		"prog", "-b", "-int=123", "--ll", "5", "-port=:8080", // "-h",
		"hi", "-unkown", "abc"
	};
	size_t argc = sizeof(argv) / sizeof(argv[0]);

	enum FlagErr err = flag_parse(flags, argc, argv);

	CU_ASSERT_FALSE(err); if (err != FlagOK) { printf("got err: %s\n", flag_err_str(err)); return; }

	CU_ASSERT_EQUAL(b, true);              if (b != true) printf("b not equal to true, is %s!\n", b ? "true" : "false");
	CU_ASSERT_EQUAL(i, 123);               if (i != 123) printf("i not equal to 123, is %d!\n", i);
	CU_ASSERT_EQUAL(l, 5);                 if (l != 5) printf("l not equal to 5, is %lld!\n", l);
	CU_ASSERT_STRING_EQUAL(port, ":8080"); if (strcmp(port, ":8080")) printf("port not equal to ':8080', is %s!\n", port);
	CU_ASSERT(flags->parsed);              if (!flags->parsed) puts("flag_parsed should be true");

	CU_ASSERT_EQUAL(flags->argc, 4);                if (flags->argc != 4) printf("argc should be 4, is %lu\n", flags->argc);
	CU_ASSERT_STRING_EQUAL(flags->argv[0], "prog"); if (strcmp(flags->argv[0], "prog")) printf("argv[0] should be 'prog', is %s\n", flags->argv[0]);
	CU_ASSERT_STRING_EQUAL(flags->argv[1], "hi");   if (strcmp(flags->argv[1], "hi")) printf("argv[1] should be 'hi', is %s\n", flags->argv[1]);

	free(flags);
}

int main()
{
	CU_ErrorCode err = CU_initialize_registry();
	if (err) {
		perror("error inititalizing test registry");
		return EXIT_FAILURE;
	}

	CU_pSuite suite = CU_add_suite("flag_test", NULL, NULL);
	CU_ADD_TEST(suite, parse_example);

	err = CU_basic_run_tests();
	if (err) {
		perror("error running tests");
		return EXIT_FAILURE;
	}

	CU_cleanup_registry();
	return EXIT_SUCCESS;
}
