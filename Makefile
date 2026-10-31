.POSIX:

# Miniaudio can use more advanced vectorisation with later versions of C. If
# your toolchain doesn't support the latest standards, you can use CSTD=C99
CC = cc
CSTD = c23
CFLAGS = -std=${CSTD} -Wall -Wextra -O2
CPPFLAGS =

OBJ = build/hkmp.o \
      build/ulepipe.o \
      build/miniaudio.o
OUT = hkmp

CTL_OBJ = build/hkmpctl.o \
          build/ulepipe.o
CTL_OUT = hkmpctl

include config.mk

all: ${OUT} ${CTL_OUT}

${OUT}: ${OBJ}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJ} ${LIBS}

${CTL_OUT}: ${CTL_OBJ}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${CTL_OBJ}

build/hkmp.o: hkmp.c ulepipe.h miniaudio.h
	mkdir -p build
	${CC} ${CPPFLAGS} ${CFLAGS} -c hkmp.c -o $@

build/hkmpctl.o: hkmpctl.c ulepipe.h
	mkdir -p build
	${CC} ${CPPFLAGS} ${CFLAGS} -c hkmpctl.c -o $@

build/ulepipe.o: ulepipe.c ulepipe.h
	mkdir -p build
	${CC} ${CPPFLAGS} ${CFLAGS} -c ulepipe.c -o $@

build/miniaudio.o: miniaudio.c miniaudio.h
	mkdir -p build
	${CC} ${CPPFLAGS} ${CFLAGS} -c miniaudio.c -o $@

clean:
	rm -f ${OUT} ${OBJ} ${CTL_OUT} ${CTL_OBJ}
	rmdir build

compile_flags:
	printf '%s\n' ${CFLAGS} > compile_flags.txt

