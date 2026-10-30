#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "miniaudio.h"
#include "ulepipe.h"

typedef enum {
	EC_OK,
} ErrorCode;

static void arguments(int argc, const char* argv[]);
static void die(ErrorCode ec, const char* fmt, ...);

static void
arguments(int argc, const char* argv[])
{
	if (argc < 2)
		return;
	
	if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
		return;
}

static void
die(ErrorCode ec, const char* fmt, ...)
{
	// XXX
	exit(ec);
}

int
main(int argc, char* argv[])
{
	arguments(argc, argv);
	return EXIT_SUCCESS;
}

