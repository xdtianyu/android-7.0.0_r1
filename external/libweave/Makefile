# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Run make with BUILD_MODE=Release for release.
BUILD_MODE ?= Debug

DEFS_Debug := \
	-D_DEBUG

DEFS_Release := \
	-DNDEBUG

INCLUDES := \
	-I. \
	-Iinclude \
	-Ithird_party/chromium \
	-Ithird_party/include \
	-Ithird_party/libuweave \
	-Ithird_party/modp_b64/modp_b64

CFLAGS := \
	-fno-exceptions \
	-fPIC \
	-fvisibility=hidden \
	-Wall \
	-Werror \
	-Wextra \
	-Wformat=2 \
	-Wl,--exclude-libs,ALL \
	-Wno-char-subscripts \
	-Wno-missing-field-initializers \
	-Wno-unused-local-typedefs \
	-Wno-unused-parameter \
	-Wpacked \
	-Wpointer-arith \
	-Wwrite-strings

CFLAGS_Debug := \
	-O0 \
	-g3

CFLAGS_Release := \
	-Os

CFLAGS_C := \
	-std=c99

CFLAGS_CC := \
	-std=c++11

comma := ,
ifeq (1, $(CLANG))
  CC = $(shell which clang-3.6)
  CXX = $(shell which clang++-3.6)
  CFLAGS := $(filter-out -Wl$(comma)--exclude-libs$(comma)ALL,$(CFLAGS))
  CFLAGS += \
    -fno-omit-frame-pointer \
    -Wno-deprecated-register \
    -Wno-inconsistent-missing-override
  ifeq (Debug, $(BUILD_MODE))
    CFLAGS += \
      -fsanitize=address
    LDFLAGS += \
      -fsanitize=address
  endif
endif

# Headers dependencies.
CFLAGS += -MMD
OBJFILES = $(shell find out/$(BUILD_MODE)/ -type f -name '*.o')
-include $(OBJFILES:.o=.d)

DEFS_TEST := \
	$(DEFS_$(BUILD_MODE)) \
	-DHAS_GTEST=1

###
# libweave.so

out/$(BUILD_MODE)/libweave.so : out/$(BUILD_MODE)/libweave_common.a
	$(CXX) -shared -Wl,-soname=libweave.so -o $@ -Wl,--whole-archive $^ -Wl,--no-whole-archive -lcrypto -lexpat -lpthread -lrt

include file_lists.mk third_party/third_party.mk examples/examples.mk tests.mk

###
# src/

weave_obj_files := $(WEAVE_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(weave_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/libweave_common.a : $(weave_obj_files) $(third_party_chromium_base_obj_files) $(third_party_chromium_crypto_obj_files) $(third_party_modp_b64_obj_files) $(third_party_libuweave_obj_files)
	rm -f $@
	$(AR) crsT $@ $^

all : out/$(BUILD_MODE)/libweave.so all-examples out/$(BUILD_MODE)/libweave_exports_testrunner out/$(BUILD_MODE)/libweave_testrunner

clean :
	rm -rf out

cleanall : clean clean-gtest clean-libevhtp

.PHONY : clean cleanall all
.DEFAULT_GOAL := all

