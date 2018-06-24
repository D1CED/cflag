// Jannis M. Hoffmann, 6.2018

/*
 * Header flag provides easy flag definition.
 */

#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<stdio.h>

enum Type {BOOL, STRING, INT, DOUBLE};

union Gen {
	char* c;
	bool b;
	int i;
	double d;
};

typedef struct Flag {
	char name[25];
	enum Type t;
	union {char c[50]; bool b; int i; double d;} val;
	char desc[100];
} flag_t;

static struct {
	unsigned int count;
	flag_t* vals[20];
} _flags;

static char* flagargs   = NULL;
static int flagargc     = 0;
static bool flagsparsed = false;

// union {char* c; bool b; int i; double d;}
// long long defval
void* _initflag(const char* name, union Gen defval, const char* desc, enum Type t)
{
	_flags.vals[_flags.count++] = calloc(sizeof(flag_t), 1);
	strncpy(_flags.vals[_flags.count-1]->name, name, 25-1);
	strncpy(_flags.vals[_flags.count-1]->desc, desc, 100-1);
	switch (t) {
	case BOOL:
		_flags.vals[_flags.count-1]->t = BOOL;
		_flags.vals[_flags.count-1]->val.b = defval.b;
		break; 
	case INT:
		_flags.vals[_flags.count-1]->t = INT;
		_flags.vals[_flags.count-1]->val.i = defval.i;
		break;
	case DOUBLE:
		_flags.vals[_flags.count-1]->t = DOUBLE;
		_flags.vals[_flags.count-1]->val.d = defval.d;
		break;
	case STRING:
		_flags.vals[_flags.count-1]->t = STRING;
		strncpy(_flags.vals[_flags.count-1]->val.c, defval.c, 50-1);
	}
	return &_flags.vals[_flags.count-1]->val;
}

// flagint lets you declare an integer flag.
int* flagint(const char* name, int defval, const char* desc)
{
	return _initflag(name, (union Gen){.i = defval}, desc, INT);
}

bool* flagbool(const char* name, bool defval, const char* desc)
{
	return _initflag(name, (union Gen){.b = defval}, desc, BOOL);
}

char* flagstring(const char* name, char* defval, const char* desc)
{
	return _initflag(name, (union Gen){.c = defval}, desc, STRING);
}

double* flagdouble(const char* name, double defval, const char* desc)
{
	return _initflag(name, (union Gen){.d = defval}, desc, DOUBLE);
}

void _printflags(const char* prog)
{
	printf("Usage of %s:\n", prog);
	for (int i = 0; i < _flags.count-2; i++) {
		flag_t* f = _flags.vals[i];
		switch (f->t) {
		case BOOL:
			printf("  -%s\n", f->name);
			printf("\t%s %s\n", f->desc, f->val.b ? "(default true)" : "");
			break;
		case INT:
			printf("  -%s %s\n", f->name, "int");
			printf("\t%s %s\n", f->desc, f->val.i ? "(default n)" : "" );
			break;
		case STRING:
			printf("  -%s %s\n", f->name, "string");
			printf("\t%s (default %s)\n", f->desc, strcmp(f->val.c, "") ? "" : f->val.c);
			break;
		case DOUBLE:
			printf("  -%s %s\n", f->name, "double");
			printf("\t%s (default %f)\n", f->desc, f->val.d);
		}
	}
	exit(2);
}

int flagparse(int argc, char** args)
{
	bool* h    = flagbool("h", false, "");
	bool* help = flagbool("help", false, "");

	int i;

	for (i = 1; i < argc; i++) {
		char key[25]  = {0};
		flag_t* match = NULL;

		// - or --
		if (args[i][0] != '-') return 1;
		char* c = args[i]+1;
		if (c[0] == '-') c++;

		char* end = strchr(c, '=');
		strncpy(key, c, end ? end-c : 25-1);
		for (int j = 0; j < _flags.count; j++) {
			if (!strcmp(_flags.vals[j]->name, key))
				match = _flags.vals[j];
		}
		if (end) end++;
		if (!match) return 1;

		// argument (=* or second)
		char* src = end;
		if (end == NULL && i+1 < argc) src = args[++i];
		if (match->t != BOOL && src == NULL) return 1;
		switch (match->t) {
		case BOOL:
			if (!end) {
				match->val.b = true;
				break;
			}
			if (!strcmp(end, "true")) match->val.b = true;
			if (!strcmp(end, "false")) match->val.b = false;
			break;
		case DOUBLE:
			match->val.d = atof(src);
			break;
		case INT:
			match->val.i = atoi(src);
			break;
		case STRING:
			strncpy(match->val.c, src, 50-1);
		}
	}
	if (*h || *help) _printflags(args[0]);
	free(_flags.vals[_flags.count--]);
	free(_flags.vals[_flags.count--]);

	flagsparsed = true;
	flagargc = argc - i;
	flagargs = args[i];

	return 0;
}
