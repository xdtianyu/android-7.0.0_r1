# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

GBMTEST = gbmtest
SOURCES += gbmtest.c

OBJS = $(SOURCES:.c=.o)
DEPS = $(SOURCES:.c=.d)

PKG_CONFIG ?= pkg-config
CCFLAGS += -g -O2 -Wall -Werror
CCFLAGS += $(shell $(PKG_CONFIG) --cflags gbm libdrm)
LDLIBS += $(PC_LIBS)
LDLIBS += $(shell $(PKG_CONFIG) --libs gbm libdrm)

.PHONY: all clean

all: $(GBMTEST)

$(GBMTEST): $(OBJS)

clean:
	$(RM) $(GBMTEST)
	$(RM) $(OBJS) $(DEPS)
	$(RM) *.o *.d .version

$(GBMTEST):
	$(CC) $(CCFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(OBJS): %.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@ -MMD

-include $(DEPS)
