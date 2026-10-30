#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>

#include "ulepipe.h"

int
ulepipe_open(ulepipe* pipe, const char* path)
{
	int sverrno = 0;
	int fd = -1;

	if (!pipe || !path) {
		errno = EINVAL;
		return -1;
	}

	pipe->fd = -1;

	if (strlen(path) >= sizeof(pipe->addr.sun_path)) {
		errno = ENAMETOOLONG;
		return -1;
	}

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (fd == -1)
		return -1;

	memset(&pipe->addr, 0, sizeof(pipe->addr));
	pipe->addr.sun_family = AF_UNIX;
	memcpy(pipe->addr.sun_path, path, sizeof(pipe->addr.sun_path));

	if (bind(fd, (struct sockaddr*)&pipe->addr, sizeof(pipe->addr)) == -1) {
		sverrno = errno;
		close(fd);
		errno = sverrno;
		return -1;
	}

	pipe->fd = fd;
	return 0;
}

void
ulepipe_close(ulepipe* pipe)
{
	if (!pipe || pipe->fd < 0)
		return;

	close(pipe->fd);
	unlink(pipe->addr.sun_path);
	pipe->fd = -1;
}

ssize_t
ulepipe_send(const char* path, const void* data, size_t size)
{
	struct sockaddr_un addr = { 0 };
	int sverrno = 0;
	ssize_t n = -1;
	int fd = -1;

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

	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, path, sizeof(addr.sun_path));

	do
		n = sendto(fd, data, size, 0, (struct sockaddr*)&addr, sizeof(addr));
	while (n == -1 && errno == EINTR);

	if (n == -1) {
		sverrno = errno;
		close(fd);
		errno = sverrno;
		return -1;
	}

	close(fd);
	return n;
}

ssize_t
ulepipe_receive(ulepipe* pipe, void* data, size_t size)
{
	struct msghdr msg = { 0 };
	struct iovec iov = { 0 };
	ssize_t n = -1;

	if (!pipe || pipe->fd < 0 || (!data && size)) {
		errno = EINVAL;
		return -1;
	}

	iov.iov_base = data;
	iov.iov_len = size;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	do
		n = recvmsg(pipe->fd, &msg, 0);
	while (n == -1 && errno == EINTR);

	if (n != -1 && (msg.msg_flags & MSG_TRUNC)) {
		errno = EMSGSIZE;
		return -1;
	}

	return n;
}

