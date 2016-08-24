# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CFLAGS		= -Os -g -Wall
OUTDIR		= ..
TARGET		= nvmap_carveout_compactor
OUT		= $(OUTDIR)/$(TARGET)
SOURCE_FILES	= nvmap_carveout_compactor.c
GCC		= $(CROSS_COMPILE)gcc

all:
	$(GCC) -lEGL -lGLESv2 $(CFLAGS) $(SOURCE_FILES) -o $(OUT)

clean:
	rm -f *.o
	rm -rf $(OUT)
