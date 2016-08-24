# Copyright (C) 2008 The Android Open Source Project
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

bzip_cflags := \
    -O3 \
    -DUSE_MMAP \
    -Wno-unused-parameter \

bzlib_files := \
    blocksort.c \
    bzlib.c \
    compress.c \
    crctable.c \
    decompress.c \
    huffman.c \
    randtable.c \

include $(CLEAR_VARS)
# measurements show that the ARM version of ZLib is about x1.17 faster
# than the thumb one...
LOCAL_ARM_MODE := arm
LOCAL_MODULE := libbz
LOCAL_CFLAGS += $(bzip_cflags)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
ifeq ($(TARGET_ARCH),arm)
  LOCAL_SDK_VERSION := 9
endif
LOCAL_SRC_FILES := $(bzlib_files)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CFLAGS += $(bzip_cflags)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_MODULE := libbz
LOCAL_SRC_FILES := $(bzlib_files)
include $(BUILD_HOST_STATIC_LIBRARY)
