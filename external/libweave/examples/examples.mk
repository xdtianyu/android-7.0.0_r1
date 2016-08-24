# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

###
# examples

examples_provider_obj_files := $(EXAMPLES_PROVIDER_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

USE_INTERNAL_LIBEVHTP ?= 1

ifeq (1, $(USE_INTERNAL_LIBEVHTP))
$(examples_provider_obj_files) : third_party/include/evhtp.h
endif

$(examples_provider_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/examples_provider.a : $(examples_provider_obj_files)
	rm -f $@
	$(AR) crsT $@ $^

EXAMPLES_DAEMON_SRC_FILES := \
	examples/daemon/ledflasher/ledflasher.cc \
	examples/daemon/light/light.cc \
	examples/daemon/lock/lock.cc \
	examples/daemon/oven/oven.cc \
	examples/daemon/sample/sample.cc \
	examples/daemon/speaker/speaker.cc

examples_daemon_obj_files := $(EXAMPLES_DAEMON_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

ifeq (1, $(USE_INTERNAL_LIBEVHTP))
$(examples_daemon_obj_files) : third_party/include/evhtp.h
endif

$(examples_daemon_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

daemon_common_flags := \
	-Wl,-rpath=out/$(BUILD_MODE)/ \
	-Lthird_party/lib \
	-levent \
	-levent_openssl \
	-lpthread \
	-lavahi-common \
	-lavahi-client \
	-lexpat \
	-lcurl \
	-lssl \
	-lcrypto

daemon_deps := out/$(BUILD_MODE)/examples_provider.a out/$(BUILD_MODE)/libweave.so

ifeq (1, $(USE_INTERNAL_LIBEVHTP))
daemon_deps += third_party/lib/libevhtp.a
else
daemon_common_flags += -levhtp
endif

out/$(BUILD_MODE)/weave_daemon_ledflasher : out/$(BUILD_MODE)/examples/daemon/ledflasher/ledflasher.o $(daemon_deps)
	$(CXX) -o $@ $^ $(CFLAGS) $(daemon_common_flags)

out/$(BUILD_MODE)/weave_daemon_light : out/$(BUILD_MODE)/examples/daemon/light/light.o $(daemon_deps)
	$(CXX) -o $@ $^ $(CFLAGS) $(daemon_common_flags)

out/$(BUILD_MODE)/weave_daemon_lock : out/$(BUILD_MODE)/examples/daemon/lock/lock.o $(daemon_deps)
	$(CXX) -o $@ $^ $(CFLAGS) $(daemon_common_flags)

out/$(BUILD_MODE)/weave_daemon_oven : out/$(BUILD_MODE)/examples/daemon/oven/oven.o $(daemon_deps)
	$(CXX) -o $@ $^ $(CFLAGS) $(daemon_common_flags)

out/$(BUILD_MODE)/weave_daemon_sample : out/$(BUILD_MODE)/examples/daemon/sample/sample.o $(daemon_deps)
	$(CXX) -o $@ $^ $(CFLAGS) $(daemon_common_flags)

out/$(BUILD_MODE)/weave_daemon_speaker : out/$(BUILD_MODE)/examples/daemon/speaker/speaker.o $(daemon_deps)
	$(CXX) -o $@ $^ $(CFLAGS) $(daemon_common_flags)

all-examples : out/$(BUILD_MODE)/weave_daemon_ledflasher out/$(BUILD_MODE)/weave_daemon_light out/$(BUILD_MODE)/weave_daemon_lock out/$(BUILD_MODE)/weave_daemon_oven out/$(BUILD_MODE)/weave_daemon_sample out/$(BUILD_MODE)/weave_daemon_speaker

.PHONY : all-examples

