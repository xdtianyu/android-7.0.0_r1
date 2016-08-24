# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CFLAGS += -Wall -Werror

# Support large files and major:minor numbers
CPPFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE

OUT = $(CURDIR)
$(shell mkdir -p $(OUT))

all: $(OUT)/rootdev $(OUT)/librootdev.so.1.0

$(OUT)/rootdev: main.c $(OUT)/librootdev.so.1.0
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

$(OUT)/librootdev.so.1.0: rootdev.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -shared -fPIC \
		-Wl,-soname,librootdev.so.1 $< -o $@
	ln -s $(@F) $(OUT)/librootdev.so.1
	ln -s $(@F) $(OUT)/librootdev.so

clean:
	rm -f $(OUT)/rootdev $(OUT)/librootdev.so*

.PHONY: clean
