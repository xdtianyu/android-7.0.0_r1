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

LIBUNWIND_SRC_FILES := \
    src/libunwind.cpp \
    src/Unwind-EHABI.cpp \
    src/Unwind-sjlj.c \
    src/UnwindLevel1-gcc-ext.c \
    src/UnwindLevel1.c \
    src/UnwindRegistersSave.S \
    src/UnwindRegistersRestore.S \

LIBUNWIND_INCLUDES := \
    $(LOCAL_PATH)/include \
    external/libcxx/include \

LIBUNWIND_CPPFLAGS := \
    -std=c++14 \
    -fexceptions \
    -Wall \
    -Wextra \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -Werror \

include $(CLEAR_VARS)
LOCAL_MODULE := libunwind_llvm
LOCAL_CLANG := true
LOCAL_SRC_FILES := $(LIBUNWIND_SRC_FILES)
LOCAL_C_INCLUDES := $(LIBUNWIND_INCLUDES)
LOCAL_CPPFLAGS := $(LIBUNWIND_CPPFLAGS)
LOCAL_MODULE_TARGET_ARCH := arm
LOCAL_CXX_STL := none
LOCAL_SANITIZE := never
include $(BUILD_STATIC_LIBRARY)
