#ifndef HKMP_H
#define HKMP_H

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef SOCKET_PATH
#define SOCKET_PATH "/tmp/hkmp.communications.socket"
#endif

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

#endif

