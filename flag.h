// Jannis M. Hoffmann, <jannis@fehcom.de>, 2019/8

/*
 * This library does provide an easy way to define flags for comand line
 * parsing that is extendable.
 * This library does not allocate any memory by iteself.
 * 
 * To use:
 * First allocate and initialize a Flagset. Then register all needed flags
 * possibly using your own type by providing the apropriate functions.
 * Lastly parse them.
 *
 * This library is heavily inspired by the Go package pkg/flag.
 *
 * ISO C11 compliant
 */

#if __STDC_VERSION__ < 201112L
#error c11 required to use flag.h
#endif

#ifndef FLAG_H
#define FLAG_H

#include <stdbool.h>
#include <stddef.h>

struct Flagset;
struct Flag_;

// Use this to calculate the size of a buffer to store a flag set.
#define FLAGSET_SIZE(l) (sizeof(struct Flagset) + (l) * sizeof(struct Flag_))

// size macros
#define FLAG_DEF_VAL_SIZE_ 100
#define FLAGSET_ARGV_SIZE_ 100

typedef void (*FlagErrFunc)(struct Flagset *, const char *);
typedef void (*FlagTypeParserFunc)(struct Flagset *, void *, size_t, const char *, FlagErrFunc);
typedef bool (*FlagTypeDisplayFunc)(void *, size_t, char[static FLAG_DEF_VAL_SIZE_]);

struct Flag_ {
	void   *value;
	size_t size;
	FlagTypeParserFunc  parseFunc;
	FlagTypeDisplayFunc displayFunc;
	bool       boolFlag;
	char       typeName[30];
	char       defValue[FLAG_DEF_VAL_SIZE_];
	const char *name;
	const char *desc;
};

// Flagset holds all your defined flags and some more meta data.
struct Flagset {
	// parsed indicating wether parsing has been done. Treat this as read only.
	bool parsed;
	// a description for your application usage.
	const char *description;
	// an error callback. You can create your own. A default is set in init.
	FlagErrFunc errFunc;
	// contains the rest of the args after parse.
	size_t     argc;
	const char *argv[FLAGSET_ARGV_SIZE_];
	// private
	size_t       capacity_;
	size_t       count_;
	struct Flag_ vals_[];
};

enum FlagErr {
	FlagOK,
	// ErrFlagNoArg is returned by flag_parse if a known non-bool flag
	// is not provided an argument.
	ErrFlagNoArg,
	// ErrFlagAlreadyParsed is returned by flag_parse if it already has run.
	ErrFlagAlreadyParsed,
	// ErrInvalidFlagValue is raised if a flag is given an invalid value.
	ErrFlagInvalidValue,
	// ErrFlagUnkown is returned if a flag is encountered that is not
	// registered in the current flagset
	ErrFlagUnknown
};

// converts the enum to its string, does not need to be freed, not writeable
const char *flag_err_str(enum FlagErr);

void flag_varbool  (struct Flagset *, bool *var, const char *name, const char *desc);
void flag_varint   (struct Flagset *, int *, const char *, const char *);
void flag_varlong  (struct Flagset *, long long *, const char *, const char *);
void flag_vardouble(struct Flagset *, double *, const char *, const char *);
void flag_varstring(struct Flagset *, char *, size_t, const char *, const char *);
void flag_var(
	struct Flagset *,
	void *,
	size_t,
	// a function creating the type from a string. The provided function can
	// be called as an error callback.
	FlagTypeParserFunc,
	// a function converting the type to a string
	FlagTypeDisplayFunc,
	const char *,
	const char *);

// initializes a Flagset. Make sure enough space is allocated to hold at lease
// n flags with it.
void flag_init(struct Flagset *, size_t n, const char *desc);

// flagparse takes as arguments the command-line arguments and
// unmarshals them into the defined flags.
// If 'h' or 'help' is given the program will exit.
//
// change flagparsing so that only consecutive flags are searched
// example:
//         ./cmd -bool -str abcde -t 12:47 non-flag other -flag 28
//                                                        ~~~~~~~~
//                                                 don't take this into account
//
// add - and -- as parse stoppers
enum FlagErr flag_parse(struct Flagset *, size_t argc, const char **argv);

#ifdef FLAG_TEST
extern bool (*flag_test_zero)(void *, size_t);
extern void (*flag_test_extract_type_name)(const char *, char *restrict, size_t);
extern void (*flag_test_parse_bool)(struct Flagset *, void *, size_t, const char *, FlagErrFunc);
extern bool (*flag_test_display_bool)(void *, size_t, char[static FLAG_DEF_VAL_SIZE_]);
#endif

#endif
