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

# Dummy target to make dEQP test list generation consistent with other tests.

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

# All APIs share the same package
LOCAL_PACKAGE_NAME := com.drawelements.deqp

include $(LOCAL_PATH)/deqp_egl.mk
include $(LOCAL_PATH)/deqp_gles2.mk
include $(LOCAL_PATH)/deqp_gles3.mk
include $(LOCAL_PATH)/deqp_gles31.mk

# Make the deqp app and copy it to CTS out dir.
cts_deqp_name := com.drawelements.deqp
cts_deqp_apk := $(CTS_TESTCASES_OUT)/$(cts_deqp_name).apk
$(cts_deqp_apk): $(call intermediates-dir-for,APPS,$(cts_deqp_name))/package.apk
	$(call copy-file-to-target)
