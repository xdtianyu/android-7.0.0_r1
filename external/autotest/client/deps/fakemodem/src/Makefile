# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

PKGS := glib-2.0 dbus-glib-1
PKG_CONFIG ?= pkg-config
PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PKGS))
PKG_LIBS := $(shell $(PKG_CONFIG) --libs $(PKGS))

CFLAGS ?= -O2
CFLAGS += $(PKG_CFLAGS)
CFLAGS += -g
WARN := -Werror -Wall
CFLAGS += $(WARN)

all: fakemodem fakenet
.PHONY: all clean install

fakemodem: fakemodem.c fakemodem-dbus.h
	$(CC) -o $@ $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $< $(PKG_LIBS)

fakemodem-dbus.h: fakemodem-dbus.xml
	dbus-binding-tool --mode=glib-server --prefix=fakemodem \
		$< > $@.tmp
	mv $@.tmp $@

fakenet: fakenet.c
	$(CC) -o $@ $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)  $<

install:
	install -m 0755 -d $(DESTDIR)/bin
	install -m 0755 fakemodem fakenet $(DESTDIR)/bin

clean:
	rm -f core fakemodem fakenet *.o *.so fakemodem-dbus.h
