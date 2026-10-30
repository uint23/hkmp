.include "config.mk"

CC = cc

# Miniaudio can use more advanced vectorisation with later versions of C. If
# your toolchain doesn't support the latest standards, you can use C99
CSTD = c23

CFLAGS = -std=${CSTD} -Wall -Wextra -O2
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

all: ${OUT}
${OUT}: ${SRC} ${HDR}
	${CC} ${CFLAGS} ${LIBS} ${CPPFLAGS} ${SRC} -o ${OUT}

${CTL_OUT}: ${CTL_SRC} ${CTL_HDR}
	${CC} ${CFLAGS} ${CPPFLAGS} ${CTL_SRC} -o ${CTL_OUT}

clean:
	rm ${OUT} ${CTL_OUT}

compile_flags:
	printf '%s\n' ${CFLAGS} > compile_flags.txt

