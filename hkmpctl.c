#include "hkmp.h"
#include "ulepipe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Command {
	const char *words[3];
	int argc;
	Control type;
	const char *name;
	int print_reply;
};

static const struct Command commands[] = {
	{{"ping"}, 1, CTL_PING, "ping", 1},
	{{"quit"}, 1, CTL_QUIT, "quit", 0},
	{{"play"}, 1, CTL_PLAY, "play", 0},
	{{"pause"}, 1, CTL_PAUSE, "pause", 0},
	{{"toggle"}, 1, CTL_TOGGLE, "toggle", 0},
	{{"loop", "song"}, 2, CTL_LOOP_SONG, "loop-song", 0},
	{{"get", "file", "name"}, 3, CTL_GET_FILE_NAME, "get-file-name", 1},
	{{"get", "file", "path"}, 3, CTL_GET_FILE_PATH, "get-file-path", 1},
	{{"haxxor"}, 1, 1337, "haxxor", 0},
};

static int
usage(const char *prog, int status)
{
	fprintf(status == EXIT_SUCCESS ? stdout : stderr,
	        "%s usage:\n"
	        "\n"
	        "ping:   Ping the daemon\n"
	        "help:   Get this helpful menu\n"
	        "quit:   Stop the daemon\n"
	        "play:   Play the current song\n"
	        "pause:  Pause the current song\n"
	        "toggle: Play/Pause the current song\n"
	        "\n"
	        "loop:\n"
	        "    song:     Toggle looping the current song\n"
	        "    playlist: Toggle looping the whole playlist\n"
	        "\n"
	        "get:\n"
	        "  file:\n"
	        "    name: Get the currently playing file name\n"
	        "    path: Get the currently playing file path\n", prog);
	return status;
}

static int
request(const struct Command *command)
{
	HkmpDataLayout data = {0};
	ssize_t n;

	data.type = command->type;
	snprintf(data.name, sizeof(data.name), "%s", command->name);
	n = ulepipe_send(SOCKET_PATH, &data, sizeof(data));
	if (n < 0) {
		perror("hkmpctl: request");
		return EXIT_FAILURE;
	}
	if ((size_t)n != sizeof(data) || data.type != CTL_REPLY ||
	    (command->print_reply && !memchr(data.s, '\0', sizeof(data.s)))) {
		fputs("hkmpctl: invalid reply\n", stderr);
		return EXIT_FAILURE;
	}
	if (command->print_reply &&
	    (fputs(data.s, stdout) == EOF || fflush(stdout) == EOF)) {
		perror("hkmpctl: stdout");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int
main(int argc, char* argv[])
{
	size_t i;
	int j;

	if (argc == 1 || (argc == 2 && !strcmp(argv[1], "help")))
		return usage(argv[0], EXIT_SUCCESS);

	for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
		if (argc - 1 != commands[i].argc)
			continue;
		for (j = 0; j < commands[i].argc; j++)
			if (strcmp(argv[j + 1], commands[i].words[j]))
				break;
		if (j == commands[i].argc)
			return request(&commands[i]);
	}
	return usage(argv[0], EXIT_FAILURE);
}
