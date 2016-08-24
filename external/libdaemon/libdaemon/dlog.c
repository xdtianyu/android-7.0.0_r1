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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "dlog.h"

enum daemon_log_flags daemon_log_use = DAEMON_LOG_AUTO|DAEMON_LOG_STDERR;
const char* daemon_log_ident = NULL;

static int daemon_verbosity_level = LOG_INFO;

void daemon_set_verbosity(int verbosity_prio) {

    /* Allow using negative verbosity levels to hide _all_ messages */
    if (verbosity_prio > 0 && (verbosity_prio & LOG_PRIMASK) != LOG_PRIMASK)
        daemon_log(LOG_ERR, "The value %d is not a valid priority value", verbosity_prio);

    daemon_verbosity_level = verbosity_prio & LOG_PRIMASK;
}

void daemon_logv(int prio, const char* template, va_list arglist) {
    int saved_errno;

    saved_errno = errno;

    if (daemon_log_use & DAEMON_LOG_SYSLOG) {
        openlog(daemon_log_ident ? daemon_log_ident : "UNKNOWN", LOG_PID, LOG_DAEMON);
        vsyslog(prio | LOG_DAEMON, template, arglist);
    }

    if (prio > daemon_verbosity_level)
        goto end_daemon_logv;

    if (daemon_log_use & DAEMON_LOG_STDERR) {
        vfprintf(stderr, template, arglist);
        fprintf(stderr, "\n");
    }

    if (daemon_log_use & DAEMON_LOG_STDOUT) {
        vfprintf(stdout, template, arglist);
        fprintf(stdout, "\n");
    }

 end_daemon_logv:
    errno = saved_errno;
}

void daemon_log(int prio, const char* template, ...) {
    va_list arglist;

    va_start(arglist, template);
    daemon_logv(prio, template, arglist);
    va_end(arglist);
}

char *daemon_ident_from_argv0(char *argv0) {
    char *p;

    if ((p = strrchr(argv0, '/')))
        return p+1;

    return argv0;
}
