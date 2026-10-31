#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>

#include "miniaudio.h"
#include "ulepipe.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef VERSION
#define VERSION "  ?  " __DATE__
#endif

#define USAGE \
"%s usage:\n" \
"\t[-v || --version]: Get the program version\n" \
"\t[-h || --help]:    Get this help menu\n" \
"", argv[0]

#ifndef SOCKET_PATH
#define SOCKET_PATH "/tmp/hkmp.communications.socket"
#endif

typedef enum {
	EC_OK,
	EC_FAIL,

	EC_SIGNAL_KILL,
	EC_SIGNAL_SEGFAULT,

	EC_DECODER_LOAD_FILE,

	EC_ENGINE_INIT,

	EC_PIPE_OPEN,
	EC_PIPE_UNKNOWN_TYPE,
} ErrorCode;

typedef enum {
	CTL_REPLY,

	CTL_PING,
	CTL_QUIT,

	CTL_PAUSE,
	CTL_PLAY,
	CTL_TOGGLE,

	CTL_LOOP_SONG,
	CTL_LOOP_PLAYLIST,

	CTL_GET_FILE_NAME,
	CTL_GET_FILE_PATH,

	/* TODO */
} Control;

typedef struct {
	Control type;
	char    name[64];

	union {
		char   s[PATH_MAX];
		double d;
		int    i;
	};
} HkmpDataLayout;

typedef struct {
	ma_sound sound;
	float    volume;
	bool     playing;
	bool     looping;
	char*    name;
	char*    path;

	struct Song* next;
	struct Song* prev;
} Song;

static void arguments(int argc, char* argv[]);
static void cleanup(void);
static void send_string(const char* s);
static void run(void);
static void say(FILE* stream, bool stop, ErrorCode ec, const char* fmt, va_list va);
static void setup(void);
static void setup_engine(void);
static void signal_handler(int sig);
static void song_loop(void);
static void song_pause(void);
static void song_play(void);
static void song_toggle(void);

static int signals[] = { SIGINT, SIGHUP, SIGQUIT, SIGTERM, SIGSEGV };

ulepipe conn = { 0 };
HkmpDataLayout conn_data = { 0 };

ma_engine engine = { 0 };
ma_decoder decoder = { 0 };
Song song = { 0 };

static inline void die(ErrorCode ec, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	say(stderr, true, ec, fmt, ap);
	va_end(ap);
}

static inline void warn(ErrorCode ec, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	say(stderr, false, ec, fmt, ap);
	va_end(ap);
}

static inline void info(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	say(stdout, false, EC_OK, fmt, ap);
	va_end(ap);
}

static void
arguments(int argc, char* argv[])
{
	if (argc < 2)
		return;

	if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")) {
		info("v" VERSION);
		exit(EC_OK);
	}
	else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		info(USAGE);
		exit(EC_OK);
	}
	else {
		song.path = argv[1];
		song.name = strrchr(argv[1], '/') + 1; /* skip / */
		return;
	}
}

static void
cleanup(void)
{
	ma_sound_uninit(&song.sound);
	ma_engine_uninit(&engine);
	ulepipe_close(&conn);
}

static void
send_string(const char* s)
{
	size_t len = strlen(s);
	char err[] = "String length longer than PATH_MAX";

	if (len > PATH_MAX) {
		memcpy(conn_data.s, err, sizeof(err));
		return;
	}

	memcpy(conn_data.s, s, len);
}

static void
run(void)
{
	for (;;) {
		ulepipe_receive(&conn, &conn_data, sizeof(conn_data));

		switch (conn_data.type) {
		case CTL_PING:;
			char msg_ping[] = "Pong !";
			memcpy(conn_data.s, msg_ping, sizeof(msg_ping));
			break;

		case CTL_QUIT:
			/* stop ctl from hanging */
			conn_data.type = CTL_REPLY;
			ulepipe_reply(&conn, &conn_data, sizeof(conn_data));
			die(EC_OK, "Quit by controller request");
			break;

		case CTL_PLAY:   song_play(); break;
		case CTL_PAUSE:  song_pause(); break;
		case CTL_TOGGLE: song_toggle(); break;

		case CTL_LOOP_PLAYLIST:
		case CTL_LOOP_SONG: song_loop(); break;

		case CTL_GET_FILE_NAME:  send_string(song.name); break;
		case CTL_GET_FILE_PATH:; send_string(song.path); break;

		default:
			warn(EC_PIPE_UNKNOWN_TYPE, "Unknown control\n"
			     "\tName: \"%s\"\n" "\tType: \"%d\"",
			     conn_data.name, conn_data.type);
			break;
		}

		conn_data.type = CTL_REPLY;
		ulepipe_reply(&conn, &conn_data, sizeof(conn_data));
	}
}

static void
say(FILE* stream, bool stop, ErrorCode ec, const char* fmt, va_list ap)
{
	/* warnings ! */
	if (stream == stderr && !stop)
		fprintf(stream, "!");
	if (stream == stderr)
		fprintf(stream, "%d> ", ec);
	vfprintf(stream, fmt, ap);
	fputc('\n', stream);
	fflush(stream);

	/* XXX? cleanup */
	if (stop) {
		cleanup();
		exit(ec);
	}
}

static void
setup(void)
{
	setup_engine();
	ma_sound_init_from_file(&engine, song.path, 0u, NULL, NULL, &song.sound);

	song_play();

	/* pipe */
	int result = ulepipe_open(&conn, SOCKET_PATH);
	if (result < 0)
		die(EC_PIPE_OPEN, "Failed to open socket at \"%s\". Removing the file could help?", SOCKET_PATH);
}

static void
setup_engine(void)
{
	ma_engine_uninit(&engine);
	ma_result audio_result = ma_decoder_init_file(song.path, NULL, &decoder);
	if (audio_result != MA_SUCCESS)
		die(EC_DECODER_LOAD_FILE, "Could not load file \"%s\"", song.path);

	ma_engine_config config = ma_engine_config_init();
	config.channels = decoder.outputChannels;
	config.sampleRate = decoder.outputSampleRate;

	ma_decoder_uninit(&decoder);

	if (ma_engine_init(&config, &engine) != MA_SUCCESS)
		die(EC_ENGINE_INIT, "Failed to initialize audio engine");
}

static void
signal_handler(int sig)
{
	switch (sig) {
	case SIGINT:
	case SIGHUP:
	case SIGQUIT:
	case SIGTERM:
		die(EC_SIGNAL_KILL, "Killing signal received [%d]", sig);
		break;
	case SIGSEGV:
		die(EC_SIGNAL_SEGFAULT, "SIGSEGV signal received");
		break;
	default:
		break;
	}
}

static void
song_loop(void)
{
	song.looping = !song.looping;
	song.looping ?
		ma_sound_set_looping(&song.sound, true) :
		ma_sound_set_looping(&song.sound, false);
}

static void
song_pause(void)
{
	ma_sound_stop(&song.sound);
	song.playing = false;
}

static void
song_play(void)
{
	ma_sound_start(&song.sound);
	song.playing = true;
}

static void
song_toggle(void)
{
	song.playing = !song.playing;
	song.playing ?
		ma_sound_start(&song.sound) :
		ma_sound_stop(&song.sound);
}

int
main(int argc, char* argv[])
{
	arguments(argc, argv);

	for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); i++)
		signal(signals[i], signal_handler);

	setup();
	run();

	return EC_OK;
}

