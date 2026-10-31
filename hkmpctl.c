#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ulepipe.h"

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define USAGE \
"%s usage:\n" \
"help: Get this helpful menu\n" \
"\n" \
"ping: Ping the daemon\n" \
"quit: Stop the daemon\n" \
"\n" \
"get\n" \
"\tfile\n" \
"\t\tname: Get the currently playing file name\n" \
"\t\tpath: Get the currently playing file path\n" \
"", argv[0]

#ifndef SOCKET_PATH
#define SOCKET_PATH "/tmp/hkmp.communications.socket"
#endif

#define die_args() do { fprintf(stderr, USAGE); exit(EXIT_FAILURE); } while(0)
#define cmp(a) if (level < argc && !strcmp(argv[level], a) && ++level)
#define send(data) do { \
if (level != argc) \
	die_args(); \
if (ulepipe_send(SOCKET_PATH, &data, sizeof(data)) < 0) \
	exit(EXIT_FAILURE); \
exit(EXIT_SUCCESS); \
} while (0)

typedef enum {
	CTL_PING,
	CTL_QUIT,

	/* TODO */
	CTL_GET_FILE_NAME,
	CTL_GET_FILE_PATH,
} Control;

typedef struct {
	Control type;
	char    name[64];

	union {
		char c;
		char str[PATH_MAX];

		float  f;
		double d;

		short     s;
		int       i;
		long      l;
		long long ll;
	};
} HkmpDataLayout;

int
main(int argc, char* argv[])
{
	int level = 1;

	if (argc <= level) {
		printf(USAGE);
		return EXIT_SUCCESS;
	}
	
	cmp("ping") {
		HkmpDataLayout data = {.name = "ping", .type = CTL_PING};
		send(data);
	}

	else cmp("quit") {
		HkmpDataLayout data = {.name = "quit", .type = CTL_QUIT};
		send(data);
	}
	
	else cmp("get") {
		cmp("file") {
			cmp("name") {
				HkmpDataLayout data = {.name = "get-file-name", .type = CTL_GET_FILE_NAME};
				send(data);
			}
			else cmp("path") {
				HkmpDataLayout data = {.name = "get-file-path", .type = CTL_GET_FILE_PATH};
				send(data);
			}
		}
	}

	else cmp("haxxor") {
		HkmpDataLayout data = {.name = "haxxor_data", .type = 1337};
		send(data);
	}

	die_args();
}

