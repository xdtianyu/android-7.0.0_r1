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

# Common project flags.
bsdiff_common_cflags := \
    -D_FILE_OFFSET_BITS=64 \
    -Wall \
    -Werror \
    -Wextra \
    -Wno-unused-parameter

bsdiff_common_static_libs := \
    libbz

bsdiff_common_unittests := \
    bsdiff_unittest.cc \
    extents_file_unittest.cc \
    extents_unittest.cc \
    test_utils.cc

# "bsdiff" program.
bsdiff_shared_libs := \
    libdivsufsort64 \
    libdivsufsort

bsdiff_src_files := \
    bsdiff.cc

# "bspatch" program.
bspatch_src_files := \
    bspatch.cc \
    extents.cc \
    extents_file.cc \
    file.cc

# Target executables.

include $(CLEAR_VARS)
LOCAL_MODULE := bspatch
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    $(bspatch_src_files) \
    bspatch_main.cc
LOCAL_CFLAGS := $(bsdiff_common_cflags)
LOCAL_C_INCLUDES += external/bzip2
LOCAL_STATIC_LIBRARIES := $(bsdiff_common_static_libs)
include $(BUILD_EXECUTABLE)


# Host executables.

include $(CLEAR_VARS)
LOCAL_MODULE := bsdiff
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    $(bsdiff_src_files) \
    bsdiff_main.cc
LOCAL_CFLAGS := $(bsdiff_common_cflags)
LOCAL_C_INCLUDES += external/bzip2
LOCAL_STATIC_LIBRARIES := $(bsdiff_common_static_libs)
LOCAL_SHARED_LIBRARIES := $(bsdiff_shared_libs)
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := bspatch
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    $(bspatch_src_files) \
    bspatch_main.cc
LOCAL_CFLAGS := $(bsdiff_common_cflags)
LOCAL_C_INCLUDES += external/bzip2
LOCAL_STATIC_LIBRARIES := $(bsdiff_common_static_libs)
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := bsdiff_unittest
LOCAL_MODULE_TAGS := debug tests
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    $(bsdiff_src_files) \
    $(bspatch_src_files) \
    $(bsdiff_common_unittests) \
    testrunner.cc
LOCAL_CFLAGS := $(bsdiff_common_cflags)
LOCAL_C_INCLUDES += external/bzip2
LOCAL_STATIC_LIBRARIES := \
    $(bsdiff_common_static_libs) \
    libgtest_host \
    libgmock_host
LOCAL_SHARED_LIBRARIES := $(bsdiff_shared_libs)
include $(BUILD_HOST_EXECUTABLE)
