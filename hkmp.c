#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "miniaudio.h"
#include "ulepipe.h"

#ifndef VERSION
#define VERSION "? " __DATE__
#endif
#define USAGE \
"%s Usage:\n" \
"\t[-v || --version]: Get %s version\n", argv[0], argv[0] \

typedef enum {
	EC_OK,
} ErrorCode;

static void arguments(int argc, char* argv[]);
static void die(ErrorCode ec, const char* fmt, ...);

static void
arguments(int argc, char* argv[])
{
	if (argc < 2)
		return;
	
	if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
		die(EC_OK, "v" VERSION);
	die(EC_OK, USAGE);
}

static void
die(ErrorCode ec, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	fflush(stderr);

	fprintf(stderr, "Died with ErrorCode [%d]\n", ec);
	exit(ec);
}

int
main(int argc, char* argv[])
{
	arguments(argc, argv);
	return EC_OK;
}

