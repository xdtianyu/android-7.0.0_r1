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

LOCAL_PATH := $(call my-dir)

# Create a list of prebuilt jars relative to $(LOCAL_PATH). i.e. lib/xxx.jar
prebuilt_jar_paths := $(shell find $(LOCAL_PATH)/libs -name "*.jar" | grep -v ".source_" | sed "s,^$(LOCAL_PATH)/,,")

# Create the list of target names each prebuilt will map to. i.e. currysrc-prebuilt-xxx
prebuilt_target_names := $(foreach path, $(prebuilt_jar_paths), $(shell echo $(path) | sed "s,^libs/\(.*\)\.jar$$,currysrc-prebuilt-\1,"))

# For each data *.jar file, define a corresponding currysrc-prebuilt-* target. i.e. currysrc-prebuilt-xxx:libs/xxx.jar
prebuilt_jar_mapping := \
    $(foreach path, $(prebuilt_jar_paths), $(shell echo $(path) | sed "s,^\(libs/\(.*\)\.jar\)$$,currysrc-prebuilt-\2:\1,"))

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := $(prebuilt_jar_mapping)
include $(BUILD_HOST_PREBUILT)

# build currysrc jar
# ============================================================

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE := currysrc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_STATIC_JAVA_LIBRARIES := $(prebuilt_target_names) guavalib
LOCAL_SRC_FILES := $(call all-java-files-under, src/)
include $(BUILD_HOST_JAVA_LIBRARY)

