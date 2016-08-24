# Copyright (C) 2014 The Android Open Source Project
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
#

LOCAL_PATH := $(call my-dir)

IGNORED_WARNINGS := -Wno-sign-compare -Wno-unused-parameter

# nanopb_c library
# =======================================================
nanopb_c_src_files := \
    pb_decode.c \
    pb_encode.c

include $(CLEAR_VARS)

LOCAL_MODULE := libprotobuf-c-nano
LOCAL_MODULE_TAGS := optional
LOCAL_C_EXTENSION := .c
LOCAL_SRC_FILES := $(nanopb_c_src_files)
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/ \

LOCAL_CFLAGS := $(IGNORED_WARNINGS)

LOCAL_SDK_VERSION := 19

include $(BUILD_STATIC_LIBRARY)

# nanopb_c library with PB_ENABLE_MALLOC enabled

include $(CLEAR_VARS)

LOCAL_MODULE := libprotobuf-c-nano-enable_malloc
LOCAL_MODULE_TAGS := optional
LOCAL_C_EXTENSION := .c
LOCAL_SRC_FILES := $(nanopb_c_src_files)
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/ \

LOCAL_CFLAGS := $(IGNORED_WARNINGS) -DPB_ENABLE_MALLOC

LOCAL_SDK_VERSION := 19

include $(BUILD_STATIC_LIBRARY)

# =======================================================

# Clean temp vars
nanopb_c_src_files :=
