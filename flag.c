// Jannis M. Hoffmann, <jannis@fehcom.de>, 2019/08

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flag.h"

typedef struct Flagset Flagset;
typedef struct Flag_ Flag;

static bool zero(void *val, size_t s)
{
	const char *c = val;
	for (size_t i = 0; i < s; ++i)
		if (c[i]) {
			//printf("%lu/%lu %hhi", i, s, c[i]);
			return false;
		}
	return true;
}

static void extract_type_name(const char *inp, char *restrict buf, size_t len)
{
	size_t
		start = 0,
		end = 0;
	for (size_t i = 0; i < strlen(inp); ++i) {
		if (start == 0) {
			if (inp[i] == '`')
				start = i+1;
		} else {
			if (inp[i] == '`') {
				end = i;
				break;
			}
		}
	}
	if (start && end) {
		size_t l = end-start > len ? len : end-start;
		strncpy(buf, inp+start, l);
		buf[l] = '\0';
	}
}

static void print_flag_description(const char *desc)
{
	char buf[300] = {0};

	size_t len = strlen(desc);
	int ctr = 0;
	for (size_t i = 0; i < len; ++i) {
		if (ctr < 2) {
			if (desc[i] == '`')
				++ctr;
			else
				buf[i-ctr] = desc[i];
		} else {
			buf[i-ctr] = desc[i];
		}
	}
	printf(buf);
}

// flagsprint prints all flags and the description
static void flag_print_defaults(Flagset *fset)
{
	printf("Usage of %s:\n", fset->argv[0]);
	for (size_t i = 0; i < fset->count_; ++i) {
		Flag *p = &fset->vals_[i];

		if (p->typeName[0])
			printf("  -%s %s\n", p->name, p->typeName);
		else
			printf("  -%s\n", p->name);

		char buf[p->size]; // VLA
		p->parseFunc(fset, buf, p->size, p->defValue, NULL);
		if (!zero(buf, p->size)) { // maybe use sentinel in buf like '\0'
			printf("\t");
			print_flag_description(p->desc);
			printf(" (default: %s)\n", p->defValue);
		} else {
			printf("\t");
			print_flag_description(p->desc);
			printf("\n");
		}
	}
	if (fset->description)
		printf("\nDescription:\n%s\n", fset->description);
}

static void errfunc(Flagset *fs, const char* c)
{
	puts(c);
	puts("");
	flag_print_defaults(fs);
	exit(2);
}

// flaginit allocates space for flags and checks for duplicate entries.
// maybe add type name
static void flag_register(
	Flagset    *fset,
	void       *val,
	size_t     size,
	void       (*parse_func)(Flagset *, void *, size_t, const char *, void (*)(Flagset *, const char *)),
	bool       (*dsiplay_func)(void *, size_t, char[static 100]),
	const char *name,
	const char *desc,
	bool       bool_flag)
{
	// check for name collision
	for (size_t i = 0; i < fset->count_; i++) {
		if (strcmp(fset->vals_[i].name, name) == 0) {
			fprintf(stderr, "ERR_FLAG_ALREADY_DEFINED: %s\n", name);
			exit(2);
		}
	}
	// check for h and help
	
	if (fset->count_ >= fset->capacity_) {
		perror("WARNING: flaglimit reached; buffer overflow prevented.\n");
		perror("HINT: increase the maximum amount of flags.\n");
		exit(2);
	}
	Flag *flag = &fset->vals_[fset->count_];
	flag->value = val;
	flag->parseFunc = parse_func;
	flag->displayFunc = dsiplay_func;
	flag->boolFlag = bool_flag;
	flag->size = size;
	flag->desc = desc;
	flag->name = name;

	const size_t tn_cap = sizeof(((Flag *)0)->typeName);
	extract_type_name(desc, flag->typeName, tn_cap);

	flag->displayFunc(flag->value, flag->size, flag->defValue);

	++fset->count_;
}

static void flag_parse_bool(Flagset *fs, void *val, size_t _, const char *inp, void (*err_func)(Flagset *, const char *))
{
	bool *b = val;
	if (
		!strcmp(inp, "true") || // test case insensitive
		!strcmp(inp, "1")
	)
		*b = true;
	else if (
		!strcmp(inp, "false") ||
		!strcmp(inp, "0")
	)
		*b = false;
	else
		err_func(fs, "erradic expression for boolean value");
}


static void flag_parse_int(Flagset *fs, void *val, size_t _, const char *inp, void (*err_func)(Flagset *, const char *))
{
	int *i = val;
	int j = atoi(inp);
	if (!j && strcmp(inp, "0"))
		err_func(fs, "invalid integral value");
	*i = j;
}

static void flag_parse_long(Flagset *fs, void *val, size_t _, const char *inp, void (*err_func)(Flagset *, const char *))
{
	long long *l = val;
	*l = atoll(inp);
}

static void flag_parse_double(Flagset *fs, void *val, size_t _, const char *inp, void (*err_func)(Flagset *, const char *))
{
	double *d = val;
	*d = atof(inp);
}

static void flag_parse_string(Flagset *fs, void *val, size_t len, const char *inp, void (*err_func)(Flagset *, const char *))
{
	strncpy(val, inp, len);
}


static bool flag_display_bool(void *val, size_t _, char buf[static 100])
{
	bool b = *(bool *)val;
	if (b)
		strcpy(buf, "true");
	else
		strcpy(buf, "false");
	return false;
}

static bool flag_display_int(void *val, size_t _, char buf[static 100])
{
	snprintf(buf, 99, "%d", *(int *)val);
	return false;
}

static bool flag_display_long(void *val, size_t _, char buf[static 100])
{
	snprintf(buf, 99, "%lld", *(long long *)val);
	return false;
}

static bool flag_display_double(void *val, size_t _, char buf[static 100])
{
	snprintf(buf, 99, "%f", *(double *)val);
	return false;
}

static bool flag_display_string(void *val, size_t _, char buf[static 100])
{
	snprintf(buf, 99, "%s", (char *)val);
	return false;
}


void flag_varbool(Flagset *fset, bool *var, const char *name, const char *desc)
{
	flag_register(fset, var, sizeof(bool), flag_parse_bool, flag_display_bool, name, desc, true);
}

void flag_varint(Flagset *fset, int *var, const char *name, const char *desc)
{
	flag_register(fset, var, sizeof(int), flag_parse_int, flag_display_int, name, desc, false);
}

void flag_varlong(Flagset *fset, long long *var, const char *name, const char *desc)
{
	flag_register(fset, var, sizeof(long long), flag_parse_long, flag_display_long, name, desc, false);
}

void flag_vardouble(Flagset *fset, double *var, const char *name, const char *desc)
{
	flag_register(fset, var, sizeof(double), flag_parse_double, flag_display_double, name, desc, false);
}

void flag_varstring(Flagset *fset, char *var, size_t len, const char *name, const char *desc)
{
	flag_register(fset, var, len, flag_parse_string, flag_display_string, name, desc, false);
}

void flag_var(
	Flagset    *fset,
	void       *var,
	size_t     size,
	void       (*setter)(Flagset *, void *, size_t, const char *, void (*)(Flagset *, const char *)),
	bool       (*getter)(void *, size_t, char[static 100]),
	const char *name,
	const char *desc)
{
	flag_register(fset, var, size, setter, getter, name, desc, false);
}


void flag_init(Flagset *fset, size_t cap, const char *desc)
{
	fset->parsed = false;
	fset->description = desc;
	fset->errFunc = errfunc;
	fset->capacity_ = cap;
	fset->count_ = 0;

	for (size_t i = 0; i < cap; ++i) {
		//memset(fset->_vals[i].typeName, 0, sizeof(((Flag *)0)->typeName));
		//memset(fset->_vals[i].defValue, 0, sizeof(((Flag *)0)->defValue));
		fset->vals_[i].typeName[0] = '\0';
		fset->vals_[i].defValue[0] = '\0';
	} 
}

enum FlagErr flag_parse(Flagset *fset, size_t argc, const char **args)
{
	if (fset->parsed)
		return ErrFlagAlreadyParsed;

	fset->argv[0] = args[0];

	for (size_t i = 0; i < argc; ++i) {
		if (
			!strcmp("-h", args[i]) ||
			!strcmp("-help", args[i]) ||
			!strcmp("--h", args[i]) ||
			!strcmp("--help", args[i])
		) {
			fset->errFunc(fset, "Explicit invocation of help");
			return FlagOK;
		}
	}

	#define name_cap sizeof(((Flag *)0)->name)
	size_t i;

	for (i = 1; i < argc; ++i) {

		char key[name_cap] = {0};
		Flag *match = NULL;

		// "-" or "--"
		if (!strcmp(args[i], "-") || !strcmp(args[i], "--"))
			break;

		if (args[i][0] != '-')
			break;

		const char *nbegin = &args[i][1];
		if (*nbegin == '-')
			++nbegin;

		char *nend = strchr(nbegin, '=');
		strncpy(key, nbegin, nend ? (ptrdiff_t)(nend-nbegin) : (ptrdiff_t)(name_cap-1));

		for (size_t j = 0; j < fset->count_; j++) {
			if (!strcmp(fset->vals_[j].name, key))
				match = &fset->vals_[j];
		}
		if (!match) {
			char buf[100] = {0};
			snprintf(buf, 100-1, "unkon flag '%s'", key);
			fset->errFunc(fset, buf);
			return ErrFlagUnknown;
		}

		// special handling of bool flags
		if (match->boolFlag && nend) {
			match->parseFunc(fset, match->value, match->size, nend+1, fset->errFunc);
			continue;
		} else if (match->boolFlag) {
			match->parseFunc(fset, match->value, match->size, "true", fset->errFunc);
			continue;
		}
		
		// argument (=* or next)
		const char *arg = nend ? nend+1 : args[++i];
		if (i >= argc) {
			char buf[100] = {0};
			snprintf(buf, 100-1, "flag '%s' has no argument", key);
			fset->errFunc(fset, buf);
			return ErrFlagNoArg; // overflow
		}

		match->parseFunc(fset, match->value, match->size, arg, fset->errFunc);
	}

	// setup new argc and argv
	fset->parsed = true;
	fset->argc = argc - i + 1;
	if (fset->argc > 100) {
		perror("That's a lot of flags. Aborting!");
		exit(2);
	}
	for (size_t j = 1; j < fset->argc; ++j)
		fset->argv[j] = args[j+i-1];

	return FlagOK;
}

const char *flag_err_str(enum FlagErr fe)
{
	switch (fe) {
	case ErrFlagNoArg:
		return "ErrFlagNoArg";
	case ErrFlagAlreadyParsed:
		return "ErrFlagAlreadyParsed";
	case ErrFlagInvalidValue:
		return "ErrFlagInvalidValue";
	case ErrFlagUnknown:
		return "ErrFlagUnknown";
	default:
		perror("illegal enum member. Aborting!");
		exit(3);
	}
}

// exported for testing purposes only
bool (*flag_test_zero)(void *, size_t) = zero;
void (*flag_test_extract_type_name)(const char *, char *restrict, size_t) = extract_type_name;
void (*flag_test_parse_bool)(Flagset *, void *, size_t, const char *, void (*)(Flagset *, const char *)) = flag_parse_bool;
bool (*flag_test_display_bool)(void *, size_t, char[static 100]) = flag_display_bool;