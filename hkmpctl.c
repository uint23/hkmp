/*
  TODO
  Escape this macro hellscape
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ulepipe.h"
#include "hkmp.h"

#define USAGE \
"%s usage:\n" \
"\n" \
"ping:   Ping the daemon\n" \
"help:   Get this helpful menu\n" \
"quit:   Stop the daemon\n" \
"play:   Play the current song\n" \
"pause:  Pause the current song\n" \
"toggle: Play/Pause the current song\n" \
"\n" \
"loop:\n" \
"    song:     Toggle looping the current song\n" \
"    playlist: Toggle looping the whole playlist\n" \
"\n" \
"get:\n" \
"  file:\n" \
"    name: Get the currently playing file name\n" \
"    path: Get the currently playing file path\n" \
"", argv[0]

#ifndef SOCKET_PATH
#define SOCKET_PATH "/tmp/hkmp.communications.socket"
#endif

#define die_args() do { fprintf(stderr, USAGE); exit(EXIT_FAILURE); } while(0)
#define cmp(a) if (level < argc && !strcmp(argv[level], a) && ++level)
#define send(data) do { \
if (level != argc) \
	die_args(); \
ulepipe_send(SOCKET_PATH, &data, sizeof(data)); \
} while (0)

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
		if (data.type == CTL_REPLY)
			printf("%s", data.s);
		return EXIT_SUCCESS;
	}

	else cmp("quit") {
		HkmpDataLayout data = {.name = "quit", .type = CTL_QUIT};
		send(data);
		return EXIT_SUCCESS;
	}

	else cmp("play") {
		HkmpDataLayout data = {.name = "play", .type = CTL_PLAY};
		send(data);
		return EXIT_SUCCESS;
	}

	else cmp("pause") {
		HkmpDataLayout data = {.name = "pause", .type = CTL_PAUSE};
		send(data);
		return EXIT_SUCCESS;
	}

	else cmp("toggle") {
		HkmpDataLayout data = {.name = "toggle", .type = CTL_TOGGLE};
		send(data);
		return EXIT_SUCCESS;
	}

	else cmp("loop") {
		cmp("song") {
			HkmpDataLayout data = {.name = "loop-song", .type = CTL_LOOP_SONG};
			send(data);
			return EXIT_SUCCESS;
		}

		else cmp("playlist") {
			/* XXX */
		}
	}

	else cmp("get") {
		cmp("file") {
			cmp("name") {
				HkmpDataLayout data = {.name = "get-file-name", .type = CTL_GET_FILE_NAME};
				send(data);
				printf("%s", data.s);
				return EXIT_SUCCESS;
			}

			else cmp("path") {
				HkmpDataLayout data = {.name = "get-file-path", .type = CTL_GET_FILE_PATH};
				send(data);
				printf("%s", data.s);
				return EXIT_SUCCESS;
			}
		}

		/* else cmp("volume") { */
		/* 	HkmpDataLayout data = {.name = "file-path", .type = CTL_GET_VOLUME}; */
		/* } */
	}

	else cmp("haxxor") {
		HkmpDataLayout data = {.name = "haxxor", .type = 1337};
		send(data);
		return EXIT_SUCCESS;
	}

	die_args();
}

