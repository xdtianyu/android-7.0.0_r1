#
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
#

LOCAL_PATH := $(call my-dir)

libnativepower_CommonCFlags := -Wall -Werror -Wno-unused-parameter
libnativepower_CommonCFlags += -Wno-sign-promo  # for libchrome
libnativepower_CommonCIncludes := $(LOCAL_PATH)/../include
libnativepower_CommonSharedLibraries := \
  libbinder \
  libbinderwrapper \
  libbrillo \
  libchrome \
  libpowermanager \
  libutils \

# libnativepower shared library
# ========================================================

include $(CLEAR_VARS)
LOCAL_MODULE := libnativepower
LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS := $(libnativepower_CommonCFlags)
LOCAL_C_INCLUDES := $(libnativepower_CommonCIncludes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include
LOCAL_SHARED_LIBRARIES := $(libnativepower_CommonSharedLibraries)
LOCAL_SRC_FILES := \
  power_manager_client.cc \
  wake_lock.cc \

include $(BUILD_SHARED_LIBRARY)

# libnativepower_tests executable
# ========================================================

include $(CLEAR_VARS)
LOCAL_MODULE := libnativepower_tests
ifdef BRILLO
  LOCAL_MODULE_TAGS := eng
endif
LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS := $(libnativepower_CommonCFlags)
LOCAL_C_INCLUDES := $(libnativepower_CommonCIncludes)
LOCAL_STATIC_LIBRARIES := libgtest libBionicGtestMain
LOCAL_SHARED_LIBRARIES := \
  $(libnativepower_CommonSharedLibraries) \
  libbinderwrapper_test_support \
  libnativepower \
  libnativepower_test_support \

LOCAL_SRC_FILES := \
  power_manager_client_unittest.cc \
  wake_lock_unittest.cc \

include $(BUILD_NATIVE_TEST)
