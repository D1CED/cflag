// Jannis M. Hoffmann, <jannis@fehcom.de>, 2018/10

/*
 * Header flag provides easy flag definition.
 * First allocate and initialize a Flagset. Then register all needed flags and
 * parse them. You can register your own type by providing the apropriate functions.
 *
 * This header is heavily inspired by the Go package pkg/flag.
 *
 * ISO C11 compliant
 */

#if __STDC_VERSION__ < 201112L
#error c11 required to use flag.h
#endif

//#define _ISOC11_SOURCE 1
//#define _POSIX_C_SOURCE 200112L

#ifndef FLAG_H
#define FLAG_H

#include <stdlib.h>
#include <stdbool.h>

struct Flagset;
struct Flag_;

// Use this to calculate the size of a buffer to store a flag set.
#define FLAGSET_SIZE(l) (sizeof(struct Flagset) + (l) * sizeof(struct Flag_))

struct Flag_ {
	void   *value;
	size_t size;
	void (*parseFunc)(
		struct Flagset *,
		void *,
		size_t,
		const char *,
		void (*)(struct Flagset *, const char *)
	);
	void       (*displayFunc)(void *, size_t, char[static 100]);
	bool       boolFlag;
	char       typeName[30];
	char       defValue[100];
	const char *name;
	const char *desc;
};

struct Flagset {
	// parsed indicating wether parsing has been done. Treat this a read only.
	bool parsed;
	// a description for your application usage.
	const char *description;
	// an error callback. You can create your own. Default set in init.
	void (*errFunc)(struct Flagset *, const char *);
	// contains the rest of the args after parse.
	size_t       argc;
	const char   *argv[100];
	size_t       capacity_;
	size_t       count_;
	struct Flag_ vals_[];
};

enum FlagErr {
	FlagOK,
	// ERR_FLAG_NO_ARG is returned by flag_parse if a known non-bool flag
	// is not provided an argument.
	ErrFlagNoArg,
	// ERR_FLAG_ALREADY_PARSED is returned by flag_parse if it already has run.
	ErrFlagAlreadyParsed,
	// ERR_INVALID_FLAG_VALUE is raised if a flag is given an invalid value.
	ErrFlagInvalidValue,
	ErrFlagUnknown
};

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
	void (*)(struct Flagset *, void *, size_t, const char *, void (*)(struct Flagset *, const char *)),
	void (*)(void *, size_t, char[100]),
	const char *,
	const char *
);

void flag_init(struct Flagset *, size_t , const char *, void (*)(struct Flagset *, const char*));
enum FlagErr flag_parse(struct Flagset *, size_t argc, const char **argv);

#endif
