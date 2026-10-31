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

	EC_DEVICE_INIT,
	EC_DEVICE_START,

	EC_PIPE_OPEN,
	EC_PIPE_UNKNOWN_TYPE,
} ErrorCode;

typedef enum {
	CTL_REPLY,

	CTL_PING,
	CTL_QUIT,

	CTL_FILE_NAME,
	CTL_FILE_PATH,

	/* TODO */
	CTL_PAUSE,
	CTL_PLAY,
	CTL_STOP,
} Control;

typedef struct {
	Control type;
	char    name[64];

	union {
		char        s[PATH_MAX];
		double      d;
		int         i;
	};
} HkmpDataLayout;

static void arguments(int argc, char* argv[]);
static void cleanup_audio(void);
static void data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);
static void run(void);
static void say(FILE* stream, bool stop, ErrorCode ec, const char* fmt, va_list va);
static void setup_decoder(void);
static void setup_pipe(void);
static void signal_handler(int sig);

static int signals[] = { SIGINT, SIGHUP, SIGQUIT, SIGTERM, SIGSEGV };
static struct {
	ma_decoder       decoder;
	ma_device        device;
	ma_result        result;
	ma_device_config device_config;

	char* name;
	char* path;
} audio = { 0 };
ulepipe conn = { 0 };
HkmpDataLayout conn_data = { 0 };

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
		audio.path = argv[1];
		audio.name = strrchr(argv[1], '/') + 1;
		return;
	}
}

static void
cleanup_audio(void)
{
	ma_decoder_uninit(&audio.decoder);
	ma_device_uninit(&audio.device);
}

static void
data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count)
{
	ma_decoder* _decoder = (ma_decoder*)device->pUserData;
	if (_decoder == NULL)
		return;

	ma_decoder_read_pcm_frames(_decoder, output, frame_count, NULL);

	(void)input;
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
			die(EC_OK, "Quit by controller request");
			break;

		case CTL_FILE_NAME:;
			size_t len_file_name = strlen(audio.name);
			char err_file_name[] = "File path too long";

			if (len_file_name > PATH_MAX)
				memcpy(conn_data.s, err_file_name, sizeof(err_file_name));
			memcpy(conn_data.s, audio.name, len_file_name);
			break;

		case CTL_FILE_PATH:;
			size_t len_file_path = strlen(audio.path);
			char err_file_path[] = "File path too long";

			if (len_file_path > PATH_MAX)
				memcpy(conn_data.s, err_file_path, sizeof(err_file_name));

			memcpy(conn_data.s, audio.path, len_file_path);
			break;

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
		ulepipe_close(&conn);
		cleanup_audio();
		exit(ec);
	}
}

static void
setup_decoder(void)
{
	audio.result = ma_decoder_init_file(audio.path, NULL, &audio.decoder);
	if (audio.result != MA_SUCCESS)
		die(EC_DECODER_LOAD_FILE, "Could not load file \"%s\"", audio.path);

	audio.device_config = ma_device_config_init(ma_device_type_playback);
	audio.device_config.playback.format   = audio.decoder.outputFormat;
	audio.device_config.playback.channels = audio.decoder.outputChannels;
	audio.device_config.sampleRate        = audio.decoder.outputSampleRate;
	audio.device_config.dataCallback      = data_callback;
	audio.device_config.pUserData         = &audio.decoder;

	audio.result = ma_device_init(NULL, &audio.device_config, &audio.device);
	if (audio.result != MA_SUCCESS)
		die(EC_DEVICE_INIT, "Failed to open playback device");

	if (ma_device_start(&audio.device) != MA_SUCCESS)
		die(EC_DEVICE_START, "Failed to start playback device");

	info("Set up audio decoder");
}

static void
setup_pipe(void)
{
	int result = ulepipe_open(&conn, SOCKET_PATH);
	if (result < 0)
		die(EC_PIPE_OPEN, "Failed to open socket at \"%s\". Removing the file could help?", SOCKET_PATH);
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

int
main(int argc, char* argv[])
{
	arguments(argc, argv);
	setup_decoder();
	
	for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); i++)
		signal(signals[i], signal_handler);
	
	setup_pipe();
	run();

	return EC_OK;
}

