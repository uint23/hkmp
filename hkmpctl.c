#include "hkmp.h"
#include "ulepipe.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	const char* words[3];
	int         argc;
	Control     type;
	const char* name;
	bool        print_reply;
} Command;

static const Command commands[] = {
	/* word                      argc   type                  name                print */
	{{"ping"},                   1,     CTL_PING,             "ping",             true},
	{{"quit"},                   1,     CTL_QUIT,             "quit",             false},
	{{"play"},                   1,     CTL_PLAY,             "play",             false},
	{{"pause"},                  1,     CTL_PAUSE,            "pause",            false},
	{{"toggle"},                 1,     CTL_TOGGLE,           "toggle",           false},
	{{"loop", "song"},           2,     CTL_LOOP_SONG,        "loop-song",        false},
	{{"get", "file", "name"},    3,     CTL_GET_FILE_NAME,    "get-file-name",    true},
	{{"get", "file", "path"},    3,     CTL_GET_FILE_PATH,    "get-file-path",    true},
	{{"get", "volume"},          2,     CTL_GET_VOLUME,       "get-volume",       true},
	{{"set", "volume"},          2,     CTL_SET_VOLUME,       "set-volume",       true},
	{{"haxxor"},                 1,     1337,                 "haxxor",           false},
};

static bool pretty = false;

static int
usage(const char *prog, int status)
{
	fprintf(status == EXIT_SUCCESS ? stdout : stderr,
	        "%s usage:\n"
	        "\n"
	        "pretty: Format output for terminal usage.\n"
	        "        Don't use this if you're not looking at the output\n"
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
request(const Command* command)
{
	HkmpDataLayout data = {0};

	data.type = command->type;
	snprintf(data.name, sizeof(data.name), "%s", command->name);

	ssize_t n = ulepipe_send(SOCKET_PATH, &data, sizeof(data));
	if (n < 0) {
		perror("hkmpctl: request");
		return EXIT_FAILURE;
	}

	if ((size_t)n != sizeof(data) || data.type != CTL_REPLY ||
	    (command->print_reply && !memchr(data.s, '\0', sizeof(data.s)))) {
		fputs("hkmpctl: invalid reply\n", stderr);
		return EXIT_FAILURE;
	}

	if (command->print_reply) {
		fputs(data.s, stdout);
		fflush(stdout);
		if (pretty)
			fputs("\n", stdout);
	}

	return EXIT_SUCCESS;
}

int
main(int argc, char* argv[])
{
	if (argc == 1 || (argc == 2 && !strcmp(argv[1], "help")))
		return usage(argv[0], EXIT_SUCCESS);

	int offset = 1;
	
	if (!strcmp(argv[1], "pretty")) {
		offset++;
		pretty = true;
	}

	for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
		if (argc - offset != commands[i].argc)
			continue;

		int word_index;
		for (word_index = 0; word_index < commands[i].argc; word_index++)
			if (strcmp(argv[word_index + offset], commands[i].words[word_index]))
				break;
		if (word_index == commands[i].argc)
			return request(&commands[i]);
	}

	return usage(argv[0], EXIT_FAILURE);
}

