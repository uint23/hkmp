CC = cc

CFLAGS = -std=c99 -Wall -Wextra -02
CPPFLAGS =

SRC = hkmp.c \
      ulepipe.c \
      miniaudio.c
HDR = ulepipe.h \
      miniaudio.h
OUT = hkmp

CTL_SRC = hkmpctl.c \
          ulepipe.c
CTL_HDR = ulepipe.h
CTL_OUT = hkmpctl

all: ${OUT} ${CTL_OUT}
${OUT}: ${SRC} ${HDR}
	${CC} ${CFLAGS} ${CPPFLAGS} ${SRC} -o ${OUT}

${CTL_OUT}: ${CTL_SRC} ${CTL_HDR}
	${CC} ${CFLAGS} ${CPPFLAGS} ${CTL_SRC} -o ${CTL_OUT}

