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

rootdev_CPPFLAGS := -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
  -D_LARGEFILE64_SOURCE

rootdev_CFLAGS := -Wall -Werror -Wno-sign-compare

# Build the shared library.
include $(CLEAR_VARS)
LOCAL_MODULE := librootdev
LOCAL_CFLAGS += $(rootdev_CFLAGS)
LOCAL_CLANG := true
LOCAL_CPPFLAGS += $(rootdev_CPPFLAGS)
LOCAL_SRC_FILES := rootdev.c
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_SHARED_LIBRARY)

# Build the command line tool.
include $(CLEAR_VARS)
LOCAL_MODULE := rootdev
LOCAL_CFLAGS += $(rootdev_CFLAGS)
LOCAL_CLANG := true
LOCAL_CPPFLAGS += $(rootdev_CPPFLAGS)
LOCAL_SHARED_LIBRARIES := librootdev
LOCAL_SRC_FILES := main.c
include $(BUILD_EXECUTABLE)
