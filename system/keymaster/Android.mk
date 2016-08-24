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

LOCAL_PATH := $(call my-dir)

###
# libkeymaster_messages contains just the code necessary to communicate with a
# AndroidKeymaster implementation, e.g. one running in TrustZone.
##
include $(CLEAR_VARS)
LOCAL_MODULE:= libkeymaster_messages
LOCAL_SRC_FILES:= \
		android_keymaster_messages.cpp \
		android_keymaster_utils.cpp \
		authorization_set.cpp \
		keymaster_tags.cpp \
		logger.cpp \
		serializable.cpp
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include
LOCAL_CFLAGS = -Wall -Werror -Wunused -DKEYMASTER_NAME_TAGS
LOCAL_CLANG := true
# TODO(krasin): reenable coverage flags, when the new Clang toolchain is released.
# Currently, if enabled, these flags will cause an internal error in Clang.
LOCAL_CLANG_CFLAGS += -fno-sanitize-coverage=edge,indirect-calls,8bit-counters,trace-cmp
LOCAL_MODULE_TAGS := optional
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
include $(BUILD_SHARED_LIBRARY)

###
# libkeymaster1 contains almost everything needed for a keymaster1
# implementation, lacking only a subclass of the (abstract) KeymasterContext
# class to provide environment-specific services and a wrapper to translate from
# the function-based keymaster HAL API to the message-based AndroidKeymaster API.
###
include $(CLEAR_VARS)
LOCAL_MODULE:= libkeymaster1
LOCAL_SRC_FILES:= \
		aes_key.cpp \
		aes_operation.cpp \
		android_keymaster.cpp \
		android_keymaster_messages.cpp \
		android_keymaster_utils.cpp \
		asymmetric_key.cpp \
		asymmetric_key_factory.cpp \
		attestation_record.cpp \
		auth_encrypted_key_blob.cpp \
		ec_key.cpp \
		ec_key_factory.cpp \
		ecdsa_operation.cpp \
		ecies_kem.cpp \
		hkdf.cpp \
		hmac.cpp \
		hmac_key.cpp \
		hmac_operation.cpp \
		integrity_assured_key_blob.cpp \
		iso18033kdf.cpp \
		kdf.cpp \
		key.cpp \
		keymaster_enforcement.cpp \
		nist_curve_key_exchange.cpp \
		ocb.c \
		ocb_utils.cpp \
		openssl_err.cpp \
		openssl_utils.cpp \
		operation.cpp \
		operation_table.cpp \
		rsa_key.cpp \
		rsa_key_factory.cpp \
		rsa_operation.cpp \
		symmetric_key.cpp
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include
LOCAL_SHARED_LIBRARIES := libcrypto libkeymaster_messages
LOCAL_CFLAGS = -Wall -Werror -Wunused
LOCAL_CLANG := true
LOCAL_CLANG_CFLAGS += -Wno-error=unused-const-variable -Wno-error=unused-private-field
# TODO(krasin): reenable coverage flags, when the new Clang toolchain is released.
# Currently, if enabled, these flags will cause an internal error in Clang.
LOCAL_CLANG_CFLAGS += -fno-sanitize-coverage=edge,indirect-calls,8bit-counters,trace-cmp
# Ignore benign warnings for now.
LOCAL_CLANG_CFLAGS += -Wno-error=unused-private-field
LOCAL_MODULE_TAGS := optional
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
include $(BUILD_SHARED_LIBRARY)


###
# libsoftkeymaster provides a software-based keymaster HAL implementation.
# This is used by keystore as a fallback for when the hardware keymaster does
# not support the request.
###
include $(CLEAR_VARS)
LOCAL_MODULE := libsoftkeymasterdevice
LOCAL_SRC_FILES := \
	ec_keymaster0_key.cpp \
	ec_keymaster1_key.cpp \
	ecdsa_keymaster1_operation.cpp \
	keymaster0_engine.cpp \
	keymaster1_engine.cpp \
	keymaster_configuration.cpp \
	rsa_keymaster0_key.cpp \
	rsa_keymaster1_key.cpp \
	rsa_keymaster1_operation.cpp \
	soft_keymaster_context.cpp \
	soft_keymaster_device.cpp \
	soft_keymaster_logger.cpp
LOCAL_C_INCLUDES := \
	system/security/keystore \
	$(LOCAL_PATH)/include
LOCAL_CFLAGS = -Wall -Werror -Wunused
LOCAL_CLANG := true
LOCAL_CLANG_CFLAGS += -Wno-error=unused-const-variable -Wno-error=unused-private-field
# TODO(krasin): reenable coverage flags, when the new Clang toolchain is released.
# Currently, if enabled, these flags will cause an internal error in Clang.
LOCAL_CLANG_CFLAGS += -fno-sanitize-coverage=edge,indirect-calls,8bit-counters,trace-cmp
LOCAL_SHARED_LIBRARIES := libkeymaster_messages libkeymaster1 liblog libcrypto libcutils
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
include $(BUILD_SHARED_LIBRARY)

###
# libkeymasterfiles is an empty library that exports all of the files in keymaster as includes.
###
include $(CLEAR_VARS)
LOCAL_MODULE := libkeymasterfiles
LOCAL_EXPORT_C_INCLUDE_DIRS := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/include
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
include $(BUILD_STATIC_LIBRARY)

# Unit tests for libkeymaster
include $(CLEAR_VARS)
LOCAL_MODULE := keymaster_tests
LOCAL_SRC_FILES := \
	android_keymaster_messages_test.cpp \
	android_keymaster_test.cpp \
	android_keymaster_test_utils.cpp \
	attestation_record_test.cpp \
	authorization_set_test.cpp \
	hkdf_test.cpp \
	hmac_test.cpp \
	kdf1_test.cpp \
	kdf2_test.cpp \
	kdf_test.cpp \
	key_blob_test.cpp \
	keymaster_enforcement_test.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include
LOCAL_CFLAGS = -Wall -Werror -Wunused -DKEYMASTER_NAME_TAGS
LOCAL_CLANG := true
LOCAL_CLANG_CFLAGS += -Wno-error=unused-const-variable -Wno-error=unused-private-field
# TODO(krasin): reenable coverage flags, when the new Clang toolchain is released.
# Currently, if enabled, these flags will cause an internal error in Clang.
LOCAL_CLANG_CFLAGS += -fno-sanitize-coverage=edge,indirect-calls,8bit-counters,trace-cmp
LOCAL_MODULE_TAGS := tests
LOCAL_SHARED_LIBRARIES := \
	libsoftkeymasterdevice \
	libkeymaster_messages \
	libkeymaster1 \
	libcrypto \
	libsoftkeymaster
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
include $(BUILD_NATIVE_TEST)
