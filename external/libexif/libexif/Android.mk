#
# Copyright (C) 2013 The Android Open Source Project
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
LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)

# WARNING: this makefile builds a shared library. Do not ever make it build
# a static library or otherwise statically link libexif with your code.

LOCAL_CLANG := true

LOCAL_C_INCLUDES := \
    $(TOP)/external/libexif

LOCAL_SRC_FILES:= \
    exif-byte-order.c \
    exif-content.c \
    exif-data.c \
    exif-entry.c \
    exif-format.c \
    exif-ifd.c \
    exif-loader.c \
    exif-log.c \
    exif-mem.c \
    exif-mnote-data.c \
    exif-tag.c \
    exif-utils.c \
    canon/exif-mnote-data-canon.c \
    canon/mnote-canon-entry.c \
    canon/mnote-canon-tag.c \
    olympus/exif-mnote-data-olympus.c \
    olympus/mnote-olympus-tag.c \
    olympus/mnote-olympus-entry.c \
    fuji/exif-mnote-data-fuji.c \
    fuji/mnote-fuji-entry.c \
    fuji/mnote-fuji-tag.c \
    pentax/exif-mnote-data-pentax.c \
    pentax/mnote-pentax-entry.c \
    pentax/mnote-pentax-tag.c

# Because all the include statements in the header files are in double layer
# ("libexif/XXXX.h") style, we need to set the export root to the parent folder.
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/..

LOCAL_MODULE := libexif

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	liblog

LOCAL_CFLAGS += -ftrapv

include $(BUILD_SHARED_LIBRARY)

# WARNING: this makefile builds a shared library. Do not ever make it build
# a static library or otherwise statically link libexif with your code.

