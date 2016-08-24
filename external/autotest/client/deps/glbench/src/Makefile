# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# TODO(ihf): When X on Nyan is gone, simplify this makefile,
#            specifically the *_PORTABLE and *_X_ONLY bits.

USE_X = $(filter X, $(USE))
ifeq ($(GRAPHICS_BACKEND),OPENGLES)
    USE_GLES = y
else
    USE_GLES = $(filter opengles, $(USE))
endif

ifneq ($(USE_X),)
    ifneq ($(USE_GLES),)
        PLATFORM = PLATFORM_X11_EGL
    else
        PLATFORM = PLATFORM_GLX
    endif
else
    PLATFORM = PLATFORM_NULL
endif

SOURCES_GL_BENCH = main.cc yuvtest.cc testbase.cc
SOURCES_GL_BENCH += glinterfacetest.cc contexttest.cc swaptest.cc
SOURCES_GL_BENCH += readpixeltest.cc
SOURCES_GL_BENCH += attributefetchtest.cc varyingsandddxytest.cc cleartest.cc
SOURCES_GL_BENCH += texturetest.cc texturereusetest.cc textureupdatetest.cc
SOURCES_GL_BENCH += textureuploadtest.cc trianglesetuptest.cc fillratetest.cc
SOURCES_GL_BENCH += windowmanagercompositingtest.cc
SOURCES_GL_BENCH += md5.cc png_helper.cc utils.cc waffle_stuff.cc

SOURCES_WINDOWMANAGERTEST = windowmanagertest.cc utils.cc waffle_stuff.cc

PKG_CONFIG ?= pkg-config
BASE_VER ?= 369476
PC_DEPS = libchrome-$(BASE_VER) libpng
PC_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PC_DEPS))
PC_LIBS := $(shell $(PKG_CONFIG) --libs $(PC_DEPS))

CXXFLAGS = -g -Wall -Werror -std=gnu++11
CPPFLAGS += $(PC_CFLAGS)
LDLIBS = $(PC_LIBS) -lgflags
# To compile outside of chroot or with newest libchrome, use the following two lines:
#CPPFLAGS += -I$(HOME)/chromium/src -DWORKAROUND_CROSBUG14304
#LDLIBS += -lbase_static -L$(HOME)/chromium/src/out/Release/obj.target/base

GL_BENCH = ../glbench
WINDOWMANAGERTEST = ../windowmanagertest

PLATFORM_PKGS = waffle-1
ifeq ($(PLATFORM),PLATFORM_GLX)
    PLATFORM_PKGS += x11
endif
PLATFORM_CFLAGS = -DPLATFORM=$(PLATFORM)
PLATFORM_CFLAGS += $(shell $(PKG_CONFIG) --cflags $(PLATFORM_PKGS))
PLATFORM_LIBS = $(shell $(PKG_CONFIG) --libs $(PLATFORM_PKGS))

ifneq ($(USE_GLES),)
CPPFLAGS += -DUSE_OPENGLES
LDLIBS += -lGLESv2 -lEGL
else
CPPFLAGS += -DUSE_OPENGL
LDLIBS += -lGL
endif

SOURCES_ALL = $(sort $(SOURCES_GL_BENCH) \
                     $(SOURCES_WINDOWMANAGERTEST))

OBJS_GL_BENCH = $(SOURCES_GL_BENCH:.cc=.o)
OBJS_WINDOWMANAGERTEST = $(SOURCES_WINDOWMANAGERTEST:.cc=.o)
OBJS_ALL = $(SOURCES_ALL:.cc=.o)
DEPS_ALL = $(SOURCES_ALL:.cc=.d)

.PHONY: all clean

EXE_PORTABLE = $(GL_BENCH) $(WINDOWMANAGERTEST)
OBJ_PORTABLE = $(sort $(OBJS_GL_BENCH) $(OBJS_WINDOWMANAGERTEST))

all:: $(EXE_PORTABLE)
ifneq ($(USE_X),)
all:: $(EXE_X_ONLY)
endif

$(GL_BENCH): $(OBJS_GL_BENCH)
$(WINDOWMANAGERTEST): $(OBJS_WINDOWMANAGERTEST)

clean:
	$(RM) $(GL_BENCH) $(WINDOWMANAGERTEST)
	$(RM) $(OBJS_ALL) $(DEPS_ALL)
	$(RM) *.o *.d .version

$(EXE_PORTABLE):
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS) $(PLATFORM_LIBS)

$(EXE_X_ONLY):
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS) -lX11 -lrt

$(OBJ_PORTABLE): %.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(PLATFORM_CFLAGS) -c $< -o $@ -MMD

$(OBJS_X_ONLY): %.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@ -MMD

-include $(DEPS_ALL)
