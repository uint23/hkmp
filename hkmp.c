#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "miniaudio.h"
#include "ulepipe.h"

#ifndef VERSION
#define VERSION "  ?  " __DATE__
#endif
#define USAGE \
"%s usage:\n" \
"\t[-v || --version]: Get the program version\n" \
"\t[-h || --help]:    Get this help menu\n" \
"", argv[0]

typedef enum {
	EC_OK,
	EC_FAIL,
	EC_DECODER_LOAD_FILE,
	EC_DEVICE_INIT, EC_DEVICE_START,
} ErrorCode;

static void arguments(int argc, char* argv[]);
static void data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);
static void die(ErrorCode ec, const char* fmt, ...);
static void run(void);
static void setup_decoder(void);

static ma_decoder decoder = { 0 };
static ma_device device = { 0 };
static ma_device_config device_config = { 0 };
static ma_result result = 0;

static const char* audio_file = NULL;

static void
arguments(int argc, char* argv[])
{
	if (argc < 2)
		return;

	if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
		die(EC_OK, "v" VERSION);
	else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
		die(EC_OK, USAGE);
	else {
		audio_file = argv[1];
		return;
	}

}

static void
data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count)
{
	ma_decoder* _decoder = (ma_decoder*)device->pUserData;
	if (_decoder == NULL) {
		return;
	}

	ma_decoder_read_pcm_frames(_decoder, output, frame_count, NULL);

	(void)input;
}

static void
die(ErrorCode ec, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	fprintf(stderr, "<%d> ", ec);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	fflush(stderr);

	fprintf(stderr, "Died with ErrorCode [%d]\n", ec);
	exit(ec);
}

static void
run(void)
{
	for (;;)
		;
}

static void setup_decoder(void)
{
	result = ma_decoder_init_file(audio_file, NULL, &decoder);
	if (result != MA_SUCCESS)
		die(EC_DECODER_LOAD_FILE, "Could not load file %s", audio_file);

	device_config = ma_device_config_init(ma_device_type_playback);
	device_config.playback.format   = decoder.outputFormat;
	device_config.playback.channels = decoder.outputChannels;
	device_config.sampleRate        = decoder.outputSampleRate;
	device_config.dataCallback      = data_callback;
	device_config.pUserData         = &decoder;

	result = ma_device_init(NULL, &device_config, &device);
	if (result != MA_SUCCESS) {
		ma_decoder_uninit(&decoder);
		die(EC_DEVICE_INIT, "Failed to open playback device");
	}

	if (ma_device_start(&device) != MA_SUCCESS) {
		ma_device_uninit(&device);
		ma_decoder_uninit(&decoder);
		die(EC_DEVICE_START, "Failed to start playback device");
	}
}

int
main(int argc, char* argv[])
{
	arguments(argc, argv);
	setup_decoder();
	run();

	return EC_OK;
}

