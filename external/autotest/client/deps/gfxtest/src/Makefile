# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CFLAGS ?= -O2 -g
CFLAGS += -Wall -Werror

getfb: getfb.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o ../$@ $(LDLIBS)
