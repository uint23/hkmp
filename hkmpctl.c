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
"\thelp: Get this helpful menu\n" \
"\n" \
"\tquit: Stop the daemon\n" \
"\tping: Ping the daemon\n" \
"", argv[0]

#ifndef SOCKET_PATH
#define SOCKET_PATH "/tmp/hkmp.communications.socket"
#endif

#define cmp(a, b) if (!(strcmp(a, b)))
#define send(data) ulepipe_send(SOCKET_PATH, &data, sizeof(data))

typedef enum {
	CTL_PING,
	CTL_QUIT,

	/* TODO */
	CTL_FILE_PLAY,
	CTL_FILE_NEXT,
	CTL_FILE_PREV,
	CTL_AUDIO_PLAY_PAUSE,
	CTL_AUDIO_SKIP,
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
	if (argc < 2) {
		printf(USAGE);
		return EXIT_SUCCESS;
	}
	
	cmp(argv[1], "ping") {
		HkmpDataLayout data = {.name = "ping", .type = CTL_PING};
		send(data);
	}

	cmp(argv[1], "quit") {
		HkmpDataLayout data = {.name = "quit", .type = CTL_QUIT};
		send(data);
	}

	cmp(argv[1], "haxxor") {
		HkmpDataLayout data = {.name = "haxxor_data", .type = 1337};
		send(data);
	}

	return EXIT_SUCCESS;
}

