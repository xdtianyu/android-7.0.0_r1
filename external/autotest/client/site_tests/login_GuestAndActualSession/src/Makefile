# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ifndef SYSROOT
  $(error Define SYSROOT)
endif

OUT_DIR ?= .
PROTO_PATH = $(SYSROOT)/usr/share/protofiles
PROTO_DEFS = $(PROTO_PATH)/chrome_device_policy.proto \
	$(PROTO_PATH)/device_management_backend.proto
PROTO_BINDINGS = $(OUT_DIR)/chrome_device_policy_pb2.py \
	$(OUT_DIR)/device_management_backend_pb2.py

all: $(PROTO_BINDINGS)

$(PROTO_BINDINGS): $(PROTO_DEFS)
	protoc --proto_path=$(PROTO_PATH) --python_out=$(OUT_DIR) $(PROTO_DEFS)

clean:
	rm -f $(PROTO_BINDINGS)
