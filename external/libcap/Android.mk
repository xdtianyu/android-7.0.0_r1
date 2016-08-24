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

LOCAL_PATH:= $(call my-dir)

libcap_src_files := \
    libcap/cap_alloc.c \
    libcap/cap_extint.c \
    libcap/cap_file.c \
    libcap/cap_flag.c \
    libcap/cap_proc.c \
    libcap/cap_text.c


libcap_cflags := -Wno-unused-parameter -Wno-tautological-compare

# Shared library.
include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_CFLAGS := $(libcap_cflags)
LOCAL_SRC_FILES := $(libcap_src_files)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/libcap/include

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/libcap/include
LOCAL_MODULE := libcap
include $(BUILD_SHARED_LIBRARY)


# Static library.
include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_CFLAGS := $(libcap_cflags)
LOCAL_SRC_FILES := $(libcap_src_files)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/libcap/include

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/libcap/include
LOCAL_MODULE := libcap
include $(BUILD_STATIC_LIBRARY)

libcap_src_files :=
