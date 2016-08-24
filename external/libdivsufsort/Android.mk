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

libdivsufsort_src_files := \
    lib/divsufsort.c \
    lib/sssort.c \
    lib/trsort.c \
    lib/utils.c

libdivsufsort_c_includes := \
    $(LOCAL_PATH)/android_include \
    $(LOCAL_PATH)/include

libdivsufsort_export_c_include_dirs := $(LOCAL_PATH)/android_include

libdivsufsort_cflags := \
    -Wall \
    -Werror \
    -Wextra \
    -DHAVE_CONFIG_H=1

# libdivsufsort using 32-bit integers for the suffix array (host shared lib)
include $(CLEAR_VARS)
LOCAL_MODULE := libdivsufsort
LOCAL_SRC_FILES := $(libdivsufsort_src_files)
LOCAL_C_INCLUDES := $(libdivsufsort_c_includes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(libdivsufsort_export_c_include_dirs)
LOCAL_CFLAGS := $(libdivsufsort_cflags)
include $(BUILD_HOST_SHARED_LIBRARY)

# libdivsufsort using 64-bit integers for the suffix array (host shared lib)
include $(CLEAR_VARS)
LOCAL_MODULE := libdivsufsort64
LOCAL_SRC_FILES := $(libdivsufsort_src_files)
LOCAL_C_INCLUDES := $(libdivsufsort_c_includes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(libdivsufsort_export_c_include_dirs)
LOCAL_CFLAGS := \
    $(libdivsufsort_cflags) \
    -DBUILD_DIVSUFSORT64
include $(BUILD_HOST_SHARED_LIBRARY)
