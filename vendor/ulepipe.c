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

#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>

#include "ulepipe.h"

int
ulepipe_open(ulepipe* conn, const char* path)
{
	int sverrno = 0;
	int fd = -1;

	if (!conn || !path) {
		errno = EINVAL;
		return -1;
	}

	conn->fd = -1;
	conn->peer_len = 0;

	if (strlen(path) >= sizeof(conn->addr.sun_path)) {
		errno = ENAMETOOLONG;
		return -1;
	}

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (fd == -1)
		return -1;

	memset(&conn->addr, 0, sizeof(conn->addr));
	conn->addr.sun_family = AF_UNIX;
	memcpy(conn->addr.sun_path, path, strlen(path) + 1);

	if (bind(fd, (struct sockaddr*)&conn->addr, sizeof(conn->addr)) == -1) {
		sverrno = errno;
		close(fd);
		errno = sverrno;
		return -1;
	}

	conn->fd = fd;
	return 0;
}

void
ulepipe_close(ulepipe* conn)
{
	if (!conn || conn->fd < 0)
		return;

	close(conn->fd);
	unlink(conn->addr.sun_path);
	conn->fd = -1;
	conn->peer_len = 0;
}

ssize_t
ulepipe_send(const char* path, void* data, size_t size)
{
	struct sockaddr_un addr = { 0 };
	struct sockaddr_un client = { 0 };
	struct msghdr msg = { 0 };
	struct iovec iov = { 0 };
	struct timeval timeout = { ULEPIPE_TIMEOUT, 0 };
	char dir[64] = { 0 };
	int sverrno = 0;
	ssize_t n = -1;
	int fd = -1;
	unsigned int i;

	if (!path || (!data && size)) {
		errno = EINVAL;
		return -1;
	}

	if (strlen(path) >= sizeof(addr.sun_path)) {
		errno = ENAMETOOLONG;
		return -1;
	}

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (fd == -1)
		return -1;

	for (i = 0; i < 1000; i++) {
		snprintf(dir, sizeof(dir), "/tmp/ulepipe.%ld.%u", (long)getpid(), i);
		if (mkdir(dir, 0700) == 0)
			break;
		if (errno != EEXIST)
			goto fail;
	}
	if (i == 1000) {
		errno = EEXIST;
		goto fail;
	}

	client.sun_family = AF_UNIX;
	snprintf(client.sun_path, sizeof(client.sun_path), "%s/socket", dir);
	if (bind(fd, (struct sockaddr*)&client, sizeof(client)) == -1 ||
	    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
		goto done;

	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, path, strlen(path) + 1);

	do
		n = sendto(fd, data, size, 0, (struct sockaddr*)&addr, sizeof(addr));
	while (n == -1 && errno == EINTR);

	if (n == -1)
		goto done;
	if ((size_t)n != size) {
		errno = EIO;
		n = -1;
		goto done;
	}

	iov.iov_base = data;
	iov.iov_len = size;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	do
		n = recvmsg(fd, &msg, 0);
	while (n == -1 && errno == EINTR);

	if (n != -1 && (msg.msg_flags & MSG_TRUNC)) {
		errno = EMSGSIZE;
		n = -1;
	}

done:
	sverrno = errno;
	close(fd);
	unlink(client.sun_path);
	rmdir(dir);
	errno = sverrno;
	return n;

fail:
	sverrno = errno;
	close(fd);
	errno = sverrno;
	return -1;
}

ssize_t
ulepipe_reply(const ulepipe* conn, const void* data, size_t size)
{
	ssize_t n = -1;

	if (!conn || conn->fd < 0 || !conn->peer_len || (!data && size)) {
		errno = EINVAL;
		return -1;
	}

	do
		n = sendto(conn->fd, data, size, 0,
		    (struct sockaddr*)&conn->peer, conn->peer_len);
	while (n == -1 && errno == EINTR);

	return n;
}

ssize_t
ulepipe_receive(ulepipe* conn, void* data, size_t size)
{
	struct msghdr msg = { 0 };
	struct iovec iov = { 0 };
	ssize_t n = -1;

	if (!conn || conn->fd < 0 || (!data && size)) {
		errno = EINVAL;
		return -1;
	}

	conn->peer_len = 0;
	iov.iov_base = data;
	iov.iov_len = size;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_name = &conn->peer;
	msg.msg_namelen = sizeof(conn->peer);

	do
		n = recvmsg(conn->fd, &msg, 0);
	while (n == -1 && errno == EINTR);

	if (n != -1 && (msg.msg_flags & MSG_TRUNC)) {
		errno = EMSGSIZE;
		return -1;
	}

	if (n != -1)
		conn->peer_len = msg.msg_namelen;

	return n;
}

