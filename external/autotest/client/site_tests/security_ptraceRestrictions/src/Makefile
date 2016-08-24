# Copyright (c) 2012 The Chromium OS Authors.
#
# Based on:
# http://bazaar.launchpad.net/~ubuntu-bugcontrol/qa-regression-testing/master/view/head:/scripts/kernel-security/ptrace/Makefile
# Copyright (C) 2010-2011 Canonical Ltd.
# License: GPLv3
CFLAGS=-Wall -Werror

EXECS=sleeper thread-prctl

all: $(EXECS)

clean:
	rm -f $(EXECS)

thread-%: thread-%.c
	$(CC) $(CFLAGS) -pthread -o $@ $<
