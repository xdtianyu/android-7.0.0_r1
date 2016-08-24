# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CFLAGS =
LDFLAGS =

SRC_OK = ok.c
SRC_FAIL = fail.c
SRC_OPEN = open.c

TARGET_OK = ../ok
TARGET_FAIL = ../fail
TARGET_OPEN = ../open

all: $(TARGET_OK) $(TARGET_FAIL) $(TARGET_OPEN)

$(TARGET_OK): $(SRC_OK)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_FAIL): $(SRC_FAIL)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_OPEN): $(SRC_OPEN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(TARGET_OK)
	$(RM) $(TARGET_FAIL)
	$(RM) $(TARGET_OPEN)
