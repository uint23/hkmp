#ifndef ULEUPIPE_H
#define ULEUPIPE_H

#include <stddef.h>
#include <sys/un.h>
#include <unistd.h>

typedef struct {
	int                fd;
	struct sockaddr_un addr;
} ulepipe;

/* Open a ulepipe socket */
int ulepipe_open(ulepipe* pipe, const char* path);

/* Close a ulepipe */
void ulepipe_close(ulepipe* pipe);

/* Send data to a ulepipe socket */
ssize_t ulepipe_send(const char* path, const void* data, size_t size);

/* Receive data from a ulepipe socket */
ssize_t ulepipe_receive(ulepipe* pipe, void* data, size_t size);

#endif /* ULEUPIPE_H */

