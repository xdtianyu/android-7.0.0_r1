# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TARGET_GL = ../gl_APICheck
TARGET_ES = ../gles_APICheck

ifeq ($(GRAPHICS_BACKEND), OPENGLES)
    LDFLAGS = -lX11 -lm -ldl
    SRCS = gles_APICheck.cc
    TARGET = $(TARGET_ES)
else ifeq ($(GRAPHICS_BACKEND), OPENGL)
    LDFLAGS = -lX11 -lm -lGL
    SRCS = gl_APICheck.cc
    TARGET = $(TARGET_GL)
else
    $(error GRAPHICS_BACKEND has to be either OPENGL or OPENGLES)
endif

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) -O3 -o $@ $^ -Wall $(LDFLAGS)

clean:
	$(RM) $(TARGET_GL)
	$(RM) $(TARGET_ES)
