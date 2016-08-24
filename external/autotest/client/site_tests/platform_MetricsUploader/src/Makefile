# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ifndef SYSROOT
  $(error Define SYSROOT)
endif

OUT_DIR ?= .
PROTO_PATH = $(SYSROOT)/usr/include/metrics/proto
PROTO_DEFS = $(PROTO_PATH)/*.proto
PROTO_BINDINGS = $(PROTO_DEFS:$PROTO_PATH%.proto=$OUT_DIR%_pb2.py)

all: $(PROTO_BINDINGS)

$(PROTO_BINDINGS): $(PROTO_DEFS)
	protoc --proto_path=$(PROTO_PATH) --python_out=$(OUT_DIR) $(PROTO_DEFS)

clean:
	rm -f $(PROTO_BINDINGS)
