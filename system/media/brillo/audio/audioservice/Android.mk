# Copyright 2016 The Android Open Source Project
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

audio_service_shared_libraries := \
  libbinderwrapper \
  libbrillo \
  libbrillo-binder \
  libc \
  libchrome \
  libmedia \
  libutils

# Audio service.
# =============================================================================
include $(CLEAR_VARS)
LOCAL_MODULE := brilloaudioservice
LOCAL_SRC_FILES := \
  audio_daemon.cpp \
  audio_device_handler.cpp \
  main_audio_service.cpp
LOCAL_SHARED_LIBRARIES := $(audio_service_shared_libraries)
LOCAL_CFLAGS := -Werror -Wall
LOCAL_INIT_RC := brilloaudioserv.rc
include $(BUILD_EXECUTABLE)

# Unit tests for audio device handler.
# =============================================================================
include $(CLEAR_VARS)
LOCAL_MODULE := brilloaudioservice_test
LOCAL_SRC_FILES := \
  audio_device_handler.cpp \
  test/audio_device_handler_test.cpp
LOCAL_C_INCLUDES := external/gtest/include
LOCAL_SHARED_LIBRARIES := $(audio_service_shared_libraries)
LOCAL_STATIC_LIBRARIES := \
  libBionicGtestMain \
  libchrome_test_helpers \
  libgmock
LOCAL_CFLAGS := -Werror -Wall
LOCAL_CFLAGS += -Wno-sign-compare
include $(BUILD_NATIVE_TEST)
