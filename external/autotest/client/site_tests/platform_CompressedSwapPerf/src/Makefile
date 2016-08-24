# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TARGET = hog
SRC = hog.c
CFLAGS = -std=c99 -D_GNU_SOURCE -O0
LDFLAGS = -lrt

$(TARGET): $(SRC)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(TARGET)
