/*
 * Copyright (c) 2012 The Chromium OS Authors.
 *
 * Based on:
 * http://bazaar.launchpad.net/~ubuntu-bugcontrol/qa-regression-testing/master/view/head:/scripts/kernel-security/ptrace/sleeper.c
 * Copyright 2010 Canonical, Ltd
 * License: GPLv3
 * Author: Kees Cook <kees.cook@canonical.com>
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <signal.h>

#ifndef PR_SET_PTRACER
# define PR_SET_PTRACER 0x59616d61
#endif

int main(int argc, char * argv[])
{
    long pid;

    if (argc<3) {
        fprintf(stderr,"Usage: %s TRACER_PID SLEEP_SECONDS\n", argv[0]);
        /* Without arguments, send a SIGINT to ourself so that gdb can
         * regain control without needing debugging symbols.
         */
        kill(getpid(), SIGINT);
        return 1;
    }

    pid = strtol(argv[1], NULL, 10);
    if (pid != -2) {
        if (prctl(PR_SET_PTRACER, pid, 0, 0, 0)) {
            perror("prctl");
            puts("failed");
            return 1;
        }
    }

    puts("ready");
    fflush(NULL);
    sleep(atoi(argv[2]));

    return 0;
}
