#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#define FLAG_TEST
#include "flag.h"

#define ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))
#define STR_BOOL(b) ((b) ? "true" : "false")

static void zero_test()
{
	bool b = true, c = false;
	CU_ASSERT_FALSE(flag_test_zero(&b, sizeof(b)));
	CU_ASSERT_TRUE(flag_test_zero(&c, sizeof(c)));

	long i = 0, j = 1, k = -1;
	CU_ASSERT_TRUE(flag_test_zero(&i, sizeof(i)));
	CU_ASSERT_FALSE(flag_test_zero(&j, sizeof(j)));
	CU_ASSERT_FALSE(flag_test_zero(&k, sizeof(k)));
}

static void extract_test()
{
	char buf[20] = {0};
	const char *a = "`abc`";
	flag_test_extract_type_name(a, buf, sizeof(buf));
	CU_ASSERT_STRING_EQUAL(buf, "abc");
	buf[0] = '\0';

	a = "";
	flag_test_extract_type_name(a, buf, sizeof(buf));
	CU_ASSERT_STRING_EQUAL(buf, "");
	buf[0] = '\0';

	a = "abcdef ghi jk";
	flag_test_extract_type_name(a, buf, sizeof(buf));
	CU_ASSERT_STRING_EQUAL(buf, "");
	buf[0] = '\0';

	a = "abcdef ghi `jk`";
	flag_test_extract_type_name(a, buf, sizeof(buf));
	CU_ASSERT_STRING_EQUAL(buf, "jk");
	buf[0] = '\0';

	a = "abcdef ghi `jk";
	flag_test_extract_type_name(a, buf, sizeof(buf));
	CU_ASSERT_STRING_EQUAL(buf, "");
}

static void bool_to_str()
{
	char buf[101];
	bool b = false;
	flag_test_display_bool(&b, sizeof(b), buf);
	CU_ASSERT_STRING_EQUAL(buf, "false");

	memset(buf, 0, sizeof(buf));
	b = true;
	flag_test_display_bool(&b, sizeof(b), buf);
	CU_ASSERT_STRING_EQUAL(buf, "true");
}

static void assert_err_wrapper(struct Flagset *f, const char *s)
{
	(void)f; (void)s;
	CU_FAIL("should be unreachable");
}
static int call_ctr = 0;
static int reset_call_ctr() { call_ctr = 0; return 0; }
static void inc_call_ctr(struct Flagset *f, const char *s)
{
	(void)f; (void)s;
	++call_ctr;
}

static void str_to_bool()
{
	char *inp = "true";
	bool b;
	flag_test_parse_bool(NULL, &b, sizeof(b), inp, assert_err_wrapper);
	CU_ASSERT_EQUAL(b, true);

	inp = "false";
	flag_test_parse_bool(NULL, &b, sizeof(b), inp, assert_err_wrapper);
	CU_ASSERT_EQUAL(b, false);

	CU_ASSERT_EQUAL(call_ctr, 0);
	inp = "blabla";
	flag_test_parse_bool(NULL, &b, sizeof(b), inp, inc_call_ctr);
	CU_ASSERT_EQUAL(call_ctr, 1);
}

// experimental table driven tests
static void str_to_bool_tt()
{
	struct {
		char        inp[20];
		bool        want;
		FlagErrFunc assert_func;
	} tt[] = {
		{"true", true, assert_err_wrapper},
		{"false", false, assert_err_wrapper},
		{"blabla", false, inc_call_ctr},
	};

	for (size_t i = 0; i < ARRAY_LEN(tt); ++i) {
		bool got;
		flag_test_parse_bool(NULL, &got, sizeof(got), tt[i].inp, tt[i].assert_func);
		if (got != tt[i].want) {
			CU_FAIL(got tt[i].want);
			printf("In test %zu: want %s, got %s",
				i, STR_BOOL(tt[i].want), STR_BOOL(got));
		}
	}
}

static void parse_example()
{
	struct Flagset *flags = malloc(FLAGSET_SIZE(5));
	flag_init(flags, 5, "This is a test programm");

	bool b = false;
	int i = 0;
	long long l = 1L << 33;
	char port[6] = ":80";

	flag_varbool  (flags, &b, "b", "`bool` test");
	flag_varint   (flags, &i, "int", "an `int` test");
	flag_varlong  (flags, &l, "ll", "hoho");
	flag_varstring(flags, port, sizeof(port)-1, "port", "specifiy a port");

	const char *argv[] = {
		"prog", "-b", "-int=123", "--ll", "5", "-port=:8080", //"-h",
		"hi", "-unkown", "abc"
	};
	size_t argc = sizeof(argv) / sizeof(argv[0]);

	enum FlagErr err = flag_parse(flags, argc, argv);

	CU_ASSERT_EQUAL(err, FlagOK);

	CU_ASSERT_EQUAL(b, true);
	CU_ASSERT_EQUAL(i, 123);
	CU_ASSERT_EQUAL(l, 5);
	CU_ASSERT_STRING_EQUAL(port, ":8080");
	CU_ASSERT_TRUE(flags->parsed);

	CU_ASSERT_EQUAL(flags->argc, 4);
	CU_ASSERT_STRING_EQUAL(flags->argv[0], "prog");
	CU_ASSERT_STRING_EQUAL(flags->argv[1], "hi");

	free(flags);
}

int main()
{
	CU_ErrorCode err = CU_initialize_registry();
	if (err) {
		perror("error inititalizing test registry");
		return EXIT_FAILURE;
	}

	CU_pSuite parse = CU_add_suite("flag_parse", NULL, NULL);
	CU_ADD_TEST(parse, parse_example);

	CU_pSuite misc = CU_add_suite("flag_misc", NULL, NULL);
	CU_ADD_TEST(misc, zero_test);
	CU_ADD_TEST(misc, extract_test);

	CU_pSuite str_to_x = CU_add_suite("flag_restore", NULL, reset_call_ctr);
	CU_ADD_TEST(str_to_x, str_to_bool);

	CU_pSuite x_to_str = CU_add_suite("flag_display", NULL, NULL);
	CU_ADD_TEST(x_to_str, bool_to_str);

	err = CU_basic_run_tests();
	if (err) {
		perror("error running tests");
		return EXIT_FAILURE;
	}

	CU_cleanup_registry();
	return EXIT_SUCCESS;
}
