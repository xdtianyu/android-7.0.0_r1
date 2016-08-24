# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# To dynamically link to GLES libs, export IMPORTGL=1
IMPORTGL = 0

OPTIONS = -O3 -Wall
FLAGS = -D SUPERSHAPE_HIGH_RES

TARGET_GL = SanOGL
TARGET_ES = SanOGLES
TARGET_ES_S = SanOGLES_S

ifeq ($(GRAPHICS_BACKEND), OPENGL)
    LDFLAGS = -lm -lGL
    SRCS = demo.c app-linux.c importvbo.c
    TARGET = $(TARGET_GL)
else ifeq ($(GRAPHICS_BACKEND), OPENGLES)
    FLAGS += -D SAN_ANGELES_OBSERVATION_GLES
    ifeq ($(IMPORTGL), 0)
        LDFLAGS = -lm -lGLESv2 -lEGL
        TARGET = $(TARGET_ES_S)
        FLAGS += -D DISABLE_IMPORTGL
    else
        LDFLAGS = -lm -ldl
        TARGET = $(TARGET_ES)
    endif
    SRCS = demo.c app-linux.c importgl.c matrixop.c shader.c
else
    $(error GRAPHICS_BACKEND has to be either OPENGL or OPENGLES)
endif

ifdef PKG_CONFIG
    FLAGS += $(shell $(PKG_CONFIG) --cflags waffle-1)
    LDFLAGS += $(shell $(PKG_CONFIG) --libs waffle-1)
endif

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(FLAGS) -o $@ $^ $(LDFLAGS) $(OPTIONS)

clean:
	$(RM) $(TARGET_GL)
	$(RM) $(TARGET_ES)
	$(RM) $(TARGET_ES_S)
