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

netperf_CFLAGS := \
  -D_GNU_SOURCE \
  -DHAVE_CONFIG_H \
  -include unistd.h \
  -include sys/sendfile.h \
  -Wno-sign-compare \
  -Wno-unused-parameter \

netperf_SRC_FILES := \
  src/dscp.c \
  src/netcpu_procstat.c \
  src/netlib.c \
  src/netsh.c \
  src/nettest_bsd.c \
  src/nettest_dlpi.c \
  src/nettest_omni.c \
  src/nettest_unix.c \
  src/nettest_xti.c \
  src/nettest_sctp.c \
  src/nettest_sdp.c \
  src/net_uuid.c \

include $(CLEAR_VARS)
LOCAL_MODULE := netperf
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng
LOCAL_CFLAGS := $(netperf_CFLAGS)
LOCAL_SRC_FILES := src/netperf.c $(netperf_SRC_FILES)
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := netserver
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng
LOCAL_CFLAGS := $(netperf_CFLAGS)
LOCAL_SRC_FILES := src/netserver.c $(netperf_SRC_FILES)
include $(BUILD_EXECUTABLE)
