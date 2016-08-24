# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CPPFLAGS += -Wall

# Add pci library on x86
ifneq (,$(filter $(shell uname -m),x86_64 i686))
  LDLIBS += -lpci
endif

TARGET = smm

all: $(TARGET)

clean:
	$(RM) $(TARGET)
