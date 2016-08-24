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
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-proto-files-under, .)

LOCAL_SHARED_LIBRARIES := \
    libprotobuf-cpp-lite

LOCAL_PROTOC_OPTIMIZE_TYPE := lite

LOCAL_MODULE := libvehiclenetworkproto-native
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_MODULE_TAGS := optional

LOCAL_STRIP_MODULE := keep_symbols

generated_sources_dir := $(call local-generated-sources-dir)
LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(generated_sources_dir)/proto/packages/services/Car/libvehiclenetwork/proto/

include $(BUILD_STATIC_LIBRARY)

# =======================================================

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-proto-files-under, .)

LOCAL_PROTOC_OPTIMIZE_TYPE := lite

LOCAL_PROTOC_FLAGS := --proto_path=$(LOCAL_PATH)

LOCAL_MODULE := libvehiclenetworkproto-java

LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_JAVA_LIBRARY)
