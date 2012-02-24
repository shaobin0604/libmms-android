#ifndef MYIO_H
#define MYIO_H

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int MyConnect(int s, const struct sockaddr *addr, socklen_t addrlen);
int MySend(int s, const void *data, size_t size, int flags);
int MyReceive(int s, void *data, size_t size, int flags);

int setReceiveTimeout(int s, int seconds);

#endif
