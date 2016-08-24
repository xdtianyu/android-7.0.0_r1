# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

PKG_CONFIG ?= pkg-config

CXXFLAGS += -g -Wall -Werror `$(PKG_CONFIG) --cflags glib-2.0 ibus-1.0`
LDLIBS = `$(PKG_CONFIG) --libs ibus-1.0`

ibusclient: ibusclient.cc
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $^ -o ../$@ $(LDLIBS)
