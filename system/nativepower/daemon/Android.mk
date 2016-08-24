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

nativepowerman_CommonCFlags := -Wall -Werror -Wno-unused-parameter
nativepowerman_CommonCFlags += -Wno-sign-promo  # for libchrome
nativepowerman_CommonCIncludes := \
  $(LOCAL_PATH)/../include \
  external/gtest/include \

nativepowerman_CommonSharedLibraries := \
  libbinder \
  libbinderwrapper \
  libchrome \
  libcutils \
  libpowermanager \
  libutils \

# nativepowerman executable
# ========================================================

include $(CLEAR_VARS)
# "nativepowermanager" would probably be a better name, but Android service
# names are limited to 16 characters.
LOCAL_MODULE := nativepowerman
LOCAL_REQUIRED_MODULES := nativepowerman.rc
LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS := $(nativepowerman_CommonCFlags)
LOCAL_STATIC_LIBRARIES := libnativepowerman
LOCAL_SHARED_LIBRARIES := \
  $(nativepowerman_CommonSharedLibraries) \
  libbrillo \
  libbrillo-binder \

LOCAL_SRC_FILES := main.cc

include $(BUILD_EXECUTABLE)

# nativepowerman.rc script
# ========================================================

ifdef INITRC_TEMPLATE
include $(CLEAR_VARS)
LOCAL_MODULE := nativepowerman.rc
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/$(TARGET_COPY_OUT_INITRCD)

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): $(INITRC_TEMPLATE)
	$(call generate-initrc-file,nativepowerman,,wakelock)
endif

# libnativepowerman client library (for daemon and tests)
# ========================================================

include $(CLEAR_VARS)
LOCAL_MODULE := libnativepowerman
LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS := $(nativepowerman_CommonCFlags)
LOCAL_C_INCLUDES := $(nativepowerman_CommonCIncludes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include
LOCAL_SHARED_LIBRARIES := \
  $(nativepowerman_CommonSharedLibraries) \
  libbrillo \

LOCAL_SRC_FILES := \
  BnPowerManager.cc \
  power_manager.cc \
  system_property_setter.cc \
  wake_lock_manager.cc \

include $(BUILD_STATIC_LIBRARY)

# nativepowerman_tests executable
# ========================================================

include $(CLEAR_VARS)
LOCAL_MODULE := nativepowerman_tests
ifdef BRILLO
  LOCAL_MODULE_TAGS := eng
endif
LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS := $(nativepowerman_CommonCFlags)
LOCAL_STATIC_LIBRARIES := libnativepowerman libgtest libBionicGtestMain
LOCAL_SHARED_LIBRARIES := \
  $(nativepowerman_CommonSharedLibraries) \
  libbinderwrapper_test_support \
  libnativepower_test_support \

LOCAL_SRC_FILES := \
  power_manager_unittest.cc \
  system_property_setter_stub.cc \
  wake_lock_manager_unittest.cc \

include $(BUILD_NATIVE_TEST)

# libnativepower_test_support shared library
# ========================================================

include $(CLEAR_VARS)
LOCAL_MODULE := libnativepower_test_support
LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS := $(nativepowerman_CommonCFlags)
LOCAL_C_INCLUDES := $(nativepowerman_CommonCIncludes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include
LOCAL_SHARED_LIBRARIES := $(nativepowerman_CommonSharedLibraries)
LOCAL_SRC_FILES := \
  BnPowerManager.cc \
  power_manager_stub.cc \
  wake_lock_manager.cc \
  wake_lock_manager_stub.cc \

include $(BUILD_SHARED_LIBRARY)
