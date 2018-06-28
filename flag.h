// Jannis M. Hoffmann, 6.2018

/*
 * Header flag provides easy flag definition.
 * Register flags with flag(bool|int|double|string) or flagvar*.
 * flag* will allocate space while flagvar* has to be given a storage location.
 * Use flagparse to unmarshal cli-arguments into the given flags.
 * Leftover or malformed cli-arguments are available via flagargv.
 * If the user request help via "h" or "help" the programm will exit with a
 * pretty-print of all available flags.
 * To define more than 20 flags change the MAXFLAGS macro to be greater.
 * To give a value multiple flags (like 'v' and 'verbose') use flagvar* twice
 * with the same stack variable.
 *
 * This header is heavily inspired by the Go package flag form the stdlib.
 *
 * C99 compliant
 */

#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<stdio.h>

#ifndef MAXFLAGS
#define MAXFLAGS 20
#endif

typedef enum {_BOOL, _INT, _DOUBLE, _STRING} _flagtype_t;

typedef struct {
	_flagtype_t type;
	void* value;
	char name[25];
	char desc[100];
} _flag_t;

static struct {
	size_t count;
	_flag_t* vals[MAXFLAGS+2];
} _flags;

static char** flagargv;
static int flagargc;
static bool flagsparsed;

void _flaginit(void* val, const char* name, const char* desc, _flagtype_t t)
{
	for (int i = 0; i < _flags.count; i++) {
		if (!strcmp(_flags.vals[i]->name, name)) {
			printf("FLAG_ALREADY_DEFINED %s\n", name);
		}
	}
	int i = _flags.count;
	if (i == MAXFLAGS+2) {
		printf("WARNING: flaglimit reached; buffer overflow prevented.\n");
		return;
	}
	_flags.vals[i] = calloc(sizeof(_flag_t), 1);
	_flags.vals[i]->type = t;
	_flags.vals[i]->value = val;
	strncpy(_flags.vals[i]->name, name, 25-1);
	strncpy(_flags.vals[i]->desc, desc, 100-1);
	_flags.count++;
}

bool* flagbool(const char* name, bool defval, const char* desc)
{
	bool* storage = malloc(sizeof(bool));
	*storage = defval;
	_flaginit(storage, name, desc, _BOOL);
	return storage;
}

int* flagint(const char* name, int defval, const char* desc)
{
	int* storage = malloc(sizeof(int));
	*storage = defval;
	_flaginit(storage, name, desc, _INT);
	return storage;
}

double* flagdouble(const char* name, double defval, const char* desc)
{
	double* storage = malloc(sizeof(double));
	*storage = defval;
	_flaginit(storage, name, desc, _DOUBLE);
	return storage;
}

char* flagstring(const char* name, char* defval, const char* desc)
{
	char* storage = calloc(sizeof(char), 50);
	strncpy(storage, defval, 50-1);
	_flaginit(storage, name, desc, _STRING);
	return storage;
}

void flagvarbool(bool* var, const char* name, const char* desc)
{
	_flaginit(var, name, desc, _BOOL);
}

void flagvarint(int* var, const char* name, const char* desc)
{
	_flaginit(var, name, desc, _INT);
}

void flagvardouble(double* var, const char* name, const char* desc)
{
	_flaginit(var, name, desc, _DOUBLE);
}

// flagvarstring registeres a string flag. Make sure it is at leas 50 chars wide.
void flagvarstring(char* var, const char* name, const char* desc)
{
	_flaginit(var, name, desc, _STRING);
}

void _flagsprint(const char* prog)
{
	printf("Usage of %s:\n", prog);
	for (int i = 0; i < _flags.count-2; i++) {
		_flag_t* p = _flags.vals[i];
		switch (p->type) {
		case _BOOL:
			printf("  -%s\n", p->name);
			printf("\t%s %s\n", p->desc, *(bool*)p->value ? "(default true)" : "");
			break;
		case _INT:
			printf("  -%s %s\n", p->name, "int");
			if (*(int*)p->value != 0) printf("\t%s (default %d)\n",
				p->desc, *(int*)p->value);
			else printf("\t%s\n", p->desc);
			break;
		case _DOUBLE:
			printf("  -%s %s\n", p->name, "double");
			if (*(double*)p->value != 0) printf("\t%s (default %g)\n", p->desc, *(double*)p->value);
			else printf("\t%s\n", p->desc);
			break;
		case _STRING:
			printf("  -%s %s\n", p->name, "string");
			if (strcmp(p->value, "")) printf("\t%s (default %s)\n", p->desc, (char*)p->value);
			else printf("\t%s\n", p->desc);
			break;
		}
	}
	exit(2);
}

// flagparse takes as arguments the command-line arguments and
// unmarshals them into the defined flags.
// If 'h' or 'help' is given the program will exit.
// error 1 = mismatched flag
// error 2 = no argument for non-bool flag
// error 3 = flags already parsed
int flagparse(int argc, char** args)
{
	if (flagsparsed) {
		printf("FLAGS_ALREADY_PARSED");
		return 3;
	}

	bool* h    = flagbool("h", false, "");
	bool* help = flagbool("help", false, "");

	int i;

	for (i = 1; i < argc; i++) {
		char key[25]   = {0};
		_flag_t* match = NULL;

		// "-" or "--"
		if (args[i][0] != '-') break;
		char* c = args[i]+1;
		if (c[0] == '-') c++;

		char* end = strchr(c, '=');
		strncpy(key, c, end ? end-c : 25-1);

		for (int j = 0; j < _flags.count; j++) {
			if (!strcmp(_flags.vals[j]->name, key))
				match = _flags.vals[j];
		}
		if (!match) {
			printf("FLAG_NOT_FOUND %s\n", key);
			return 1;
		}

		// argument (=* or next)
		if (end != NULL) end++;
		char* arg = end;
		if (match->type != _BOOL && end == NULL) {
			if (i+1 >= argc) {
				printf("NO_ARGUMENT for %s\n", key);
				return 2;
			}
			arg = args[++i];
		}

		switch (match->type) {
		case _BOOL:
			if (end == NULL) {
				*(bool*)match->value = true;
				break;
			}
			if (!strcmp(end, "true")) *(bool*)match->value = true;
			if (!strcmp(end, "false")) *(bool*)match->value = false;
			break;
		case _INT:
			*(int*)match->value = atoi(arg);
			break;
		case _DOUBLE:
			*(double*)match->value = atof(arg);
			break;
		case _STRING:
			strncpy(match->value, arg, 50-1);
			break;
		}
	}
	if (*h || *help) _flagsprint(args[0]);
	free(_flags.vals[--_flags.count]->value);
	free(_flags.vals[_flags.count]);
	free(_flags.vals[--_flags.count]->value);
	free(_flags.vals[_flags.count]);

	flagsparsed = true;
	flagargc    = argc - i;
	flagargv    = args + i;

	// free all flags?
	return 0;
}
