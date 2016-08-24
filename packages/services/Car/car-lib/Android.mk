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

#disble build in PDK, missing aidl import breaks build
ifneq ($(TARGET_BUILD_PDK),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := android.car
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, src) $(call all-Iaidl-files-under, src)

include $(BUILD_JAVA_LIBRARY)

ifeq ($(BOARD_IS_AUTOMOTIVE), true)
$(call dist-for-goals,dist_files,$(LOCAL_BUILT_MODULE):$(LOCAL_MODULE).jar)
endif

# API Check
# ---------------------------------------------
car_module := $(LOCAL_MODULE)
car_module_src_files := $(LOCAL_SRC_FILES)
car_module_api_dir := $(LOCAL_PATH)/api
car_module_java_libraries := framework
car_module_include_systemapi := true
car_module_java_packages := android.car*
include $(CAR_API_CHECK)

endif #TARGET_BUILD_PDK
