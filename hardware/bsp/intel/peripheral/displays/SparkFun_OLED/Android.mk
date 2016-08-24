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

# Example app to drive the SparkFun OLED Display

include $(CLEAR_VARS)
LOCAL_CPPFLAGS:= -fexceptions
LOCAL_CFLAGS += -DLOG_TAG=\"sparkFunOLED\"
LOCAL_SHARED_LIBRARIES := libcutils libupm libmraa

LOCAL_MODULE := example-display-sparkfun-oled
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := SparkFunOLED.cpp
include $(BUILD_EXECUTABLE)
