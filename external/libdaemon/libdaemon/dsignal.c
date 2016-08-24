/***
  This file is part of libdaemon.

  Copyright 2003-2008 Lennart Poettering

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>

#include "dsignal.h"
#include "dlog.h"
#include "dnonblock.h"

static int _signal_pipe[2] = { -1, -1 };

static void _sigfunc(int s) {
    int saved_errno = errno;
    write(_signal_pipe[1], &s, sizeof(s));
    errno = saved_errno;
}

static int _init(void) {

    if (_signal_pipe[0] < 0 || _signal_pipe[1] < 0) {
        if (pipe(_signal_pipe) < 0) {
            daemon_log(LOG_ERR, "pipe(): %s", strerror(errno));
            return -1;
        }

        if (daemon_nonblock(_signal_pipe[0], 1) < 0 || daemon_nonblock(_signal_pipe[1], 1) < 0) {
            daemon_signal_done();
            return -1;
        }
    }

    return 0;
}

int daemon_signal_install(int s){
    sigset_t ss;
    struct sigaction sa;

    if (_init() < 0)
        return -1;

    if (sigemptyset(&ss) < 0) {
        daemon_log(LOG_ERR, "sigemptyset(): %s", strerror(errno));
        return -1;
    }

    if (sigaddset(&ss, s) < 0) {
        daemon_log(LOG_ERR, "sigaddset(): %s", strerror(errno));
        return -1;
    }

    if (sigprocmask(SIG_UNBLOCK, &ss, NULL) < 0) {
        daemon_log(LOG_ERR, "sigprocmask(): %s", strerror(errno));
        return -1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = _sigfunc;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(s, &sa, NULL) < 0) {
        daemon_log(LOG_ERR, "sigaction(%s, ...) failed: %s", strsignal(s), strerror(errno));
        return -1;
    }

    return 0;
}

int daemon_signal_init(int s, ...) {
    int sig, r = 0;
    va_list ap;

    if (_init() < 0)
        return -1;

    va_start(ap, s);

    sig = s;
    while (sig > 0) {
        if ((r = daemon_signal_install(sig)) < 0)
            break;

        sig = va_arg(ap, int);
    }

    va_end(ap);

    return r;
}

void daemon_signal_done(void) {
    int saved_errno = errno;

    if (_signal_pipe[0] != -1)
        close(_signal_pipe[0]);

    if (_signal_pipe[1] != -1)
        close(_signal_pipe[1]);

    _signal_pipe[0] = _signal_pipe[1] = -1;

    errno = saved_errno;
}

int daemon_signal_next(void) {
    int s;
    ssize_t r;

    if ((r = read(_signal_pipe[0], &s, sizeof(s))) == sizeof(s))
        return s;

    if (r < 0) {

        if (errno == EAGAIN)
            return 0;
        else {
            daemon_log(LOG_ERR, "read(signal_pipe[0], ...): %s", strerror(errno));
            return -1;
        }
    }

    daemon_log(LOG_ERR, "Short read() on signal pipe.");
    return -1;
}

int daemon_signal_fd(void) {
    return _signal_pipe[0];
}
