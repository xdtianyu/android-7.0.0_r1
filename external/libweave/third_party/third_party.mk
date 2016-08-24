# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

###
# third_party/chromium/

third_party_chromium_base_obj_files := $(THIRD_PARTY_CHROMIUM_BASE_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_chromium_base_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

third_party_chromium_base_unittest_obj_files := $(THIRD_PARTY_CHROMIUM_BASE_UNITTEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_chromium_base_unittest_obj_files) : out/$(BUILD_MODE)/%.o : %.cc third_party/include/gtest/gtest.h
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_TEST) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

third_party_chromium_crypto_obj_files := $(THIRD_PARTY_CHROMIUM_CRYPTO_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_chromium_crypto_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

third_party_chromium_crypto_unittest_obj_files := $(THIRD_PARTY_CHROMIUM_CRYPTO_UNITTEST_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_chromium_crypto_unittest_obj_files) : out/$(BUILD_MODE)/%.o : %.cc third_party/include/gtest/gtest.h
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_TEST) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

###
# third_party/modp_b64/

third_party_modp_b64_obj_files := $(THIRD_PARTY_MODP_B64_SRC_FILES:%.cc=out/$(BUILD_MODE)/%.o)

$(third_party_modp_b64_obj_files) : out/$(BUILD_MODE)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_CC) -c -o $@ $<

###
# third_party/libuweave/

third_party_libuweave_obj_files := $(THIRD_PARTY_LIBUWEAVE_SRC_FILES:%.c=out/$(BUILD_MODE)/%.o)

$(third_party_libuweave_obj_files) : out/$(BUILD_MODE)/%.o : %.c
	mkdir -p $(dir $@)
	$(CC) $(DEFS_$(BUILD_MODE)) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(BUILD_MODE)) $(CFLAGS_C) -c -o $@ $<

###
# libgtest and libgmock (third_party, downloaded on build)

third_party/lib/gtest.a: third_party/include/gtest/gtest.h
third_party/lib/gmock.a: third_party/include/gtest/gtest.h

third_party/include/gtest/gtest.h:
	@echo Downloading and building libgtest and libgmock...
	third_party/get_gtest.sh
	@echo Finished downloading and building libgtest and libgmock.

clean-gtest :
	rm -rf third_party/include/gtest third_party/include/gmock
	rm -rf third_party/lib/libgmock* third_party/lib/libgtest*
	rm -rf third_party/googletest

###
# libevhtp (third_party, downloaded on build)

third_party/lib/libevhtp.a : third_party/include/evhtp.h
third_party/include/evhtp.h :
	@echo Downloading and building libevhtp...
	third_party/get_libevhtp.sh
	@echo Finished downloading and building libevhtp.

clean-libevhtp :
	rm -rf third_party/include/evhtp.h third_party/include/evhtp-config.h third_party/include/evthr.h third_party/include/htparse.h
	rm -rf third_party/lib/libevhtp.a
	rm -rf third_party/libevhtp
