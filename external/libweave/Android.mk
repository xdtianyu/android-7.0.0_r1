# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/file_lists.mk

# Common variables
# ========================================================

libweaveCommonCppExtension := .cc
libweaveCommonCFlags := -Wall -Werror \
	-Wno-char-subscripts -Wno-missing-field-initializers \
	-Wno-unused-function -Wno-unused-parameter

libweaveCommonCppFlags := \
	-Wno-deprecated-register \
	-Wno-sign-compare \
	-Wno-sign-promo \
	-Wno-non-virtual-dtor \

libweaveCommonCIncludes := \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/third_party/modp_b64/modp_b64 \
	$(LOCAL_PATH)/third_party/libuweave \
	external/gtest/include \

libweaveSharedLibraries := \
	libchrome \
	libexpat \
	libcrypto \

# libweave-external
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libweave-external
LOCAL_CPP_EXTENSION := $(libweaveCommonCppExtension)
LOCAL_CFLAGS := $(libweaveCommonCFlags)
LOCAL_CPPFLAGS := $(libweaveCommonCppFlags)
LOCAL_C_INCLUDES := $(libweaveCommonCIncludes)
LOCAL_SHARED_LIBRARIES := $(libweaveSharedLibraries)
LOCAL_STATIC_LIBRARIES :=
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/external

LOCAL_SRC_FILES := \
	$(THIRD_PARTY_CHROMIUM_CRYPTO_SRC_FILES) \
	$(THIRD_PARTY_MODP_B64_SRC_FILES) \
	$(THIRD_PARTY_LIBUWEAVE_SRC_FILES)

include $(BUILD_STATIC_LIBRARY)

# libweave-common
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libweave-common
LOCAL_CPP_EXTENSION := $(libweaveCommonCppExtension)
LOCAL_CFLAGS := $(libweaveCommonCFlags)
LOCAL_CPPFLAGS := $(libweaveCommonCppFlags)
LOCAL_C_INCLUDES := $(libweaveCommonCIncludes)
LOCAL_SHARED_LIBRARIES := $(libweaveSharedLibraries)
LOCAL_STATIC_LIBRARIES := libweave-external
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_SRC_FILES := $(WEAVE_SRC_FILES)

include $(BUILD_STATIC_LIBRARY)

# libweave-test
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libweave-test
LOCAL_CPP_EXTENSION := $(libweaveCommonCppExtension)
LOCAL_CFLAGS := $(libweaveCommonCFlags)
LOCAL_CPPFLAGS := $(libweaveCommonCppFlags)
LOCAL_C_INCLUDES := \
	$(libweaveCommonCIncludes) \
	external/gmock/include \

LOCAL_SHARED_LIBRARIES := $(libweaveSharedLibraries)
LOCAL_STATIC_LIBRARIES := libgtest libgmock
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_SRC_FILES := $(WEAVE_TEST_SRC_FILES)

include $(BUILD_STATIC_LIBRARY)

# libweave
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libweave
LOCAL_CPP_EXTENSION := $(libweaveCommonCppExtension)
LOCAL_CFLAGS := $(libweaveCommonCFlags)
LOCAL_CPPFLAGS := $(libweaveCommonCppFlags)
LOCAL_C_INCLUDES := $(libweaveCommonCIncludes)
LOCAL_SHARED_LIBRARIES := $(libweaveSharedLibraries)
LOCAL_WHOLE_STATIC_LIBRARIES := libweave-common libweave-external
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_SRC_FILES :=

include $(BUILD_SHARED_LIBRARY)

# libweave_test
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libweave_test
LOCAL_MODULE_TAGS := eng
LOCAL_CPP_EXTENSION := $(libweaveCommonCppExtension)
LOCAL_CFLAGS := $(libweaveCommonCFlags)
LOCAL_CPPFLAGS := $(libweaveCommonCppFlags)
LOCAL_C_INCLUDES := \
	$(libweaveCommonCIncludes) \
	external/gmock/include \

LOCAL_SHARED_LIBRARIES := \
	$(libweaveSharedLibraries) \

LOCAL_STATIC_LIBRARIES := \
	libweave-common \
	libweave-external \
	libweave-test \
	libgtest libgmock \
	libchrome_test_helpers \

LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_SRC_FILES := \
	$(WEAVE_UNITTEST_SRC_FILES) \
	$(THIRD_PARTY_CHROMIUM_CRYPTO_UNITTEST_SRC_FILES)

include $(BUILD_NATIVE_TEST)
