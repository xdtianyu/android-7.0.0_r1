# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

###
# tests

TEST_FLAGS ?= \
	--gtest_break_on_failure

TEST_ENV ?=
ifeq (1, $(CLANG))
  TEST_ENV += ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer-3.6)
endif

weave_test_obj_files := $(WEAVE_TEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(weave_test_obj_files) : out/$(BUILD_MODE)/%.o : %.cc third_party/include/gtest/gtest.h
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/libweave-test.a : $(weave_test_obj_files)
	$(AR) crs $@ $^

weave_unittest_obj_files := $(WEAVE_UNITTEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(weave_unittest_obj_files) : out/$(BUILD_MODE)/%.o : %.cc third_party/include/gtest/gtest.h
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_TEST) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/libweave_testrunner : \
	$(weave_unittest_obj_files) \
	$(third_party_chromium_crypto_unittest_obj_files) \
	$(third_party_chromium_base_unittest_obj_files) \
	out/$(BUILD_MODE)/libweave_common.a \
	out/$(BUILD_MODE)/libweave-test.a \
	third_party/lib/gmock.a \
	third_party/lib/gtest.a
	$(CXX) -o $@ $^ $(CFLAGS) -lcrypto -lexpat -lpthread -lrt -Lthird_party/lib

test : out/$(BUILD_MODE)/libweave_testrunner
	$(TEST_ENV) $< $(TEST_FLAGS)

###
# export tests

weave_exports_unittest_obj_files := $(WEAVE_EXPORTS_UNITTEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(weave_exports_unittest_obj_files) : out/$(BUILD_MODE)/%.o : %.cc third_party/include/gtest/gtest.h
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_TEST) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

out/$(BUILD_MODE)/libweave_exports_testrunner : \
	$(weave_exports_unittest_obj_files) \
	out/$(BUILD_MODE)/libweave.so \
	out/$(BUILD_MODE)/libweave-test.a \
	out/$(BUILD_MODE)/src/test/weave_testrunner.o \
	third_party/lib/gmock.a \
	third_party/lib/gtest.a
	$(CXX) -o $@ $^ $(CFLAGS) -lcrypto -lexpat -lpthread -lrt -Lthird_party/lib -Wl,-rpath=out/$(BUILD_MODE)/

export-test : out/$(BUILD_MODE)/libweave_exports_testrunner
	$(TEST_ENV) $< $(TEST_FLAGS)

testall : test export-test

.PHONY : test export-test testall

