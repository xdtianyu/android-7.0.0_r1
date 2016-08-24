# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

GPURESET = gpureset
SOURCES += gpureset.c

OBJS = $(SOURCES:.c=.o)
DEPS = $(SOURCES:.c=.d)

PKG_CONFIG ?= pkg-config
CCFLAGS += -g -O2 -Wall -Werror
CCFLAGS += $(shell $(PKG_CONFIG) --cflags libdrm)
CCFLAGS += $(shell $(PKG_CONFIG) --cflags glib-2.0)
CCFLAGS += $(shell $(PKG_CONFIG) --cflags libudev)
LDFLAGS += $(shell $(PKG_CONFIG) --libs-only-L libudev)
LDLIBS += $(PC_LIBS)
LDLIBS += $(shell $(PKG_CONFIG) --libs-only-l libudev)
INTEL_GPU := $(shell $(PKG_CONFIG) --exists libdrm_intel && echo "1" || echo "0")
ifeq ($(INTEL_GPU), 1)
  CCFLAGS += -D__INTEL_GPU__
  LDLIBS += $(shell $(PKG_CONFIG) --libs-only-l libdrm_intel)
endif

.PHONY: all clean

all: $(GPURESET)

$(GPURESET): $(OBJS)

clean:
	$(RM) $(GPURESET)
	$(RM) $(OBJS) $(DEPS)
	$(RM) *.o *.d .version

$(GPURESET):
	$(CC) $(CCFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(OBJS): %.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@ -MMD

-include $(DEPS)
