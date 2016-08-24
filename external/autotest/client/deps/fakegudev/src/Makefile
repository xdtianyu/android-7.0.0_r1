# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

PKG_CONFIG ?= pkg-config
PKGS_FAKEGUDEV := gudev-1.0 glib-2.0
PKG_CFLAGS_FAKEGUDEV := $(shell ${PKG_CONFIG} --cflags $(PKGS_FAKEGUDEV))
PKG_LIBS_FAKEGUDEV := $(shell ${PKG_CONFIG} --libs $(PKGS_FAKEGUDEV))
PKGS_FAKESYSCALLS := glib-2.0
PKG_CFLAGS_FAKESYSCALLS := $(shell ${PKG_CONFIG} --cflags $(PKGS_FAKESYSCALLS))
PKG_LIBS_FAKESYSCALLS := $(shell ${PKG_CONFIG} --libs $(PKGS_FAKESYSCALLS))
WARN := -Werror -Wall
DEBUG := -g -DFAKE_G_UDEV_DEBUG -DFAKE_SYSCALLS_DEBUG
OPT := -O2

all: libfakegudev.so gudev-exercise libfakesyscalls.so fakesyscalls-exercise
.PHONY: clean all install

gudev-exercise: gudev-exercise.c
	$(CC) -o $@ $(WARN) $(OPT) $(DEBUG) $(PKG_CFLAGS_FAKEGUDEV) \
		$< $(PKG_LIBS_FAKEGUDEV)

libfakegudev.so: fakegudev.c
	$(CC) -fPIC -shared -o $@ $(WARN) $(OPT) $(DEBUG) \
		$(PKG_CFLAGS_FAKEGUDEV) $< -ldl

fakesyscalls-exercise: fakesyscalls-exercise.c
	$(CC) -o $@ $(WARN) $(OPT) $(DEBUG) $(PKG_CFLAGS_FAKESYSCALLS) \
		$< $(PKG_LIBS_FAKESYSCALLS)

libfakesyscalls.so: fakesyscalls.c
	$(CC) -fPIC -shared -o $@ $(WARN) $(OPT) $(DEBUG) \
		 $(PKG_CFLAGS_FAKESYSCALLS) $< -ldl

install:
	install -m 0755 -d $(DESTDIR)/lib
	install -m 0755 libfakegudev.so $(DESTDIR)/lib
	install -m 0755 libfakesyscalls.so $(DESTDIR)/lib

clean:
	rm -f core gudev-exercise *.o *.so
