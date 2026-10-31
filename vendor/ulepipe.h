/* Copyright (c) uint

   Permission to use, copy, modify, and distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
   SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
   OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#ifndef ULEUPIPE_H
#define ULEUPIPE_H

#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef ULEPIPE_TIMEOUT
#define ULEPIPE_TIMEOUT 5
#endif

typedef struct {
	int       fd;
	socklen_t peer_len;

	struct sockaddr_un addr;
	struct sockaddr_un peer;
} ulepipe;

/* Open a ulepipe socket */
int ulepipe_open(ulepipe* conn, const char* path);

/* Close a ulepipe */
void ulepipe_close(ulepipe* conn);

/* Send data to a ulepipe socket */
ssize_t ulepipe_send(const char* path, void* data, size_t size);

/* Reply response to connected client */
ssize_t ulepipe_reply(const ulepipe* conn, const void* data, size_t size);

/* Receive data from a ulepipe socket */
ssize_t ulepipe_receive(ulepipe* conn, void* data, size_t size);

#endif /* ULEUPIPE_H */

