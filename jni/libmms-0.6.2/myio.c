#include "myio.h"

#define OK 0
#define UNKNOWN_ERROR -1

#define true 1
#define false 0

/********** logging **********/
#ifdef OS_ANDROID

#define ANDROID_LOG_MODULE "libmms[myio]"
#include "android-log.h"

#ifdef AACD_LOGLEVEL_DEBUG
#include <android/log.h>
#define lprintf(...) \
    __android_log_print(ANDROID_LOG_DEBUG, ANDROID_LOG_MODULE, __VA_ARGS__)
#else
#define lprintf(...) 
#endif

#else
#define lprintf(...) \
    if (getenv("LIBMMS_DEBUG")) \
        fprintf(stderr, "mms: " __VA_ARGS__)
#endif

int setReceiveTimeout(int s, int seconds) {
    if (seconds < 0) {
        // Disable the timeout.
        seconds = 0;
    }

    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec = seconds;
    return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static int MakeSocketBlocking(int s, int blocking) {
    // Make socket non-blocking.
    int flags = fcntl(s, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }

    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }

    return fcntl(s, F_SETFL, flags) != -1;
}

int MyConnect(
        int s, const struct sockaddr *addr, socklen_t addrlen) {
    int result = UNKNOWN_ERROR;

    MakeSocketBlocking(s, false);

    if (connect(s, addr, addrlen) == 0) {
        result = OK;
    } else if (errno != EINPROGRESS) {
        result = -errno;
    } else {
        for (;;) {
            fd_set rs, ws;
            FD_ZERO(&rs);
            FD_ZERO(&ws);
            FD_SET(s, &rs);
            FD_SET(s, &ws);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000ll;

            int nfds = select(s + 1, &rs, &ws, NULL, &tv);

            if (nfds < 0) {
                if (errno == EINTR) {
                    continue;
                }

                result = -errno;
                break;
            }

            if (FD_ISSET(s, &ws) && !FD_ISSET(s, &rs)) {
                result = OK;
                break;
            }

            if (FD_ISSET(s, &rs) || FD_ISSET(s, &ws)) {
                // Get the pending error.
                int error = 0;
                socklen_t errorLen = sizeof(error);
                if (getsockopt(s, SOL_SOCKET, SO_ERROR, &error, &errorLen) == -1) {
                    // Couldn't get the real error, so report why not.
                    result = -errno;
                } else {
                    result = -error;
                }
                break;
            }

            // Timeout expired. Try again.
        }
    }

    MakeSocketBlocking(s, true);

    return result;
}

// Apparently under our linux closing a socket descriptor from one thread
// will not unblock a pending send/recv on that socket on another thread.
static ssize_t MySendReceive(
        int s, void *data, size_t size, int flags, int sendData) {
    ssize_t result = 0;

    if (s < 0) {
        return -1;
    }
    while (size > 0) {
        fd_set rs, ws, es;
        FD_ZERO(&rs);
        FD_ZERO(&ws);
        FD_ZERO(&es);
        FD_SET(s, sendData ? &ws : &rs);
        FD_SET(s, &es);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000ll;

        int nfds = select(
                s + 1,
                sendData ? NULL : &rs,
                sendData ? &ws : NULL,
                &es,
                &tv);

        if (nfds < 0) {
            if (errno == EINTR) {
                continue;
            }

            result = -errno;
            break;
        } else if (nfds == 0) {
            // timeout

            continue;
        }

        if (nfds != 1) {
            lprintf("nfds != 1, abort ...\n");
        }

        ssize_t nbytes =
            sendData ? send(s, data, size, flags) : recv(s, data, size, flags);

        if (nbytes < 0) {
            if (errno == EINTR) {
                continue;
            }

            result = -errno;
            break;
        } else if (nbytes == 0) {
            result = 0;
            break;
        }

        data = (uint8_t *)data + nbytes;
        size -= nbytes;

        result = nbytes;
        break;
    }

    return result;
}

int MySend(int s, const void *data, size_t size, int flags) {
    return MySendReceive(
            s, data, size, flags, true /* sendData */);
}

int MyReceive(int s, void *data, size_t size, int flags) {
    return MySendReceive(s, data, size, flags, false /* sendData */);
}
