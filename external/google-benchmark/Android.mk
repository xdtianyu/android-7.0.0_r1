#
# Copyright (C) 2016 The Android Open Source Project
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

google_benchmark_c_flags := \
  -no-integrated-as \
  -DBENCHMARK_ANDROID \
  -DHAVE_POSIX_REGEX \

google_benchmark_src_files := \
  src/benchmark.cc \
  src/colorprint.cc \
  src/commandlineflags.cc \
  src/console_reporter.cc \
  src/csv_reporter.cc \
  src/json_reporter.cc \
  src/log.cc \
  src/reporter.cc \
  src/re_posix.cc \
  src/sleep.cc \
  src/string_util.cc \
  src/sysinfo.cc \
  src/walltime.cc \

include $(CLEAR_VARS)
LOCAL_MODULE := libgoogle-benchmark
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_CFLAGS := $(google_benchmark_c_flags)
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := $(google_benchmark_src_files)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libgoogle-benchmark
LOCAL_MODULE_HOST_OS := linux
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_CFLAGS := $(google_benchmark_c_flags)
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := $(google_benchmark_src_files)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
include $(BUILD_HOST_STATIC_LIBRARY)
