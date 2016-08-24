# Copyright (C) 2011 The Android Open Source Project
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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

# Include both the 32 and 64 bit versions
LOCAL_MULTILIB := both

LOCAL_STATIC_JAVA_LIBRARIES := ctstestserver ctstestrunner ctsdeviceutil compatibility-device-util guava

LOCAL_JAVA_LIBRARIES := android.test.runner org.apache.http.legacy

LOCAL_JNI_SHARED_LIBRARIES := libctssecurity_jni libcts_jni libnativehelper_compat_libc++ \
		libnativehelper \
		libbinder \
		libutils \
		libmedia \
		libselinux \
		libcutils \
		libcrypto \
		libc++ \
		libbacktrace \
		libui \
		libsonivox \
		libexpat \
		libcamera_client \
		libgui \
		libaudioutils \
		libnbaio \
		libpcre \
		libpackagelistparser \
		libpowermanager \
		libbase \
		libunwind \
		libhardware \
		libsync \
		libcamera_metadata \
		libspeexresampler \
		liblzma \
		libstagefright_foundation

LOCAL_SRC_FILES := $(call all-java-files-under, src)\
                   src/android/security/cts/activity/ISecureRandomService.aidl

LOCAL_PACKAGE_NAME := CtsSecurityTestCases

#LOCAL_SDK_VERSION := current

# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts

include $(BUILD_CTS_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
