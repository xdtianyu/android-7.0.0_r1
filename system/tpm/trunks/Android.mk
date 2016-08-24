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

# Common variables
# ========================================================
trunksCppExtension := .cc
trunksCFlags := -Wall -Werror -Wno-unused-parameter -DUSE_BINDER_IPC
trunksIncludes := $(LOCAL_PATH)/.. external/gtest/include
trunksSharedLibraries := \
  libbinder \
  libbinderwrapper \
  libbrillo \
  libbrillo-binder \
  libchrome \
  libchrome-crypto \
  libcrypto \
  libprotobuf-cpp-lite \
  libutils \

# libtrunks_generated
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libtrunks_generated
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_CPP_EXTENSION := $(trunksCppExtension)
LOCAL_CFLAGS := $(trunksCFlags)
LOCAL_CLANG := true
proto_include := $(call local-generated-sources-dir)/proto/$(LOCAL_PATH)/..
aidl_include := $(call local-generated-sources-dir)/aidl-generated/include
LOCAL_C_INCLUDES := $(proto_include) $(aidl_include) $(trunksIncludes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(proto_include) $(aidl_include)
LOCAL_SHARED_LIBRARIES := $(trunksSharedLibraries)
LOCAL_SRC_FILES := \
  interface.proto \
  aidl/android/trunks/ITrunks.aidl \
  aidl/android/trunks/ITrunksClient.aidl \

LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/aidl
include $(BUILD_STATIC_LIBRARY)

# libtrunks_common
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libtrunks_common
LOCAL_CPP_EXTENSION := $(trunksCppExtension)
LOCAL_CFLAGS := $(trunksCFlags)
LOCAL_CLANG := true
LOCAL_C_INCLUDES := $(trunksIncludes)
LOCAL_SHARED_LIBRARIES := $(trunksSharedLibraries)
LOCAL_STATIC_LIBRARIES := libtrunks_generated
LOCAL_SRC_FILES := \
  background_command_transceiver.cc \
  blob_parser.cc \
  error_codes.cc \
  hmac_authorization_delegate.cc \
  hmac_session_impl.cc \
  password_authorization_delegate.cc \
  policy_session_impl.cc \
  scoped_key_handle.cc \
  session_manager_impl.cc \
  tpm_generated.cc \
  tpm_state_impl.cc \
  tpm_utility_impl.cc \
  trunks_factory_impl.cc \

include $(BUILD_STATIC_LIBRARY)

# trunksd
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := trunksd
LOCAL_CPP_EXTENSION := $(trunksCppExtension)
LOCAL_CFLAGS := $(trunksCFlags)
ifeq ($(BRILLOEMULATOR),true)
LOCAL_CFLAGS += -DUSE_SIMULATOR
endif
LOCAL_CLANG := true
ifeq ($(BRILLOEMULATOR),true)
LOCAL_INIT_RC := trunksd-simulator.rc
else
LOCAL_INIT_RC := trunksd.rc
endif
LOCAL_C_INCLUDES := $(trunksIncludes)
LOCAL_SHARED_LIBRARIES := \
  $(trunksSharedLibraries) \
  libbrillo-minijail \
  libminijail \

ifeq ($(BRILLOEMULATOR),true)
LOCAL_SHARED_LIBRARIES += libtpm2
endif
LOCAL_STATIC_LIBRARIES := \
  libtrunks_generated \
  libtrunks_common \

LOCAL_REQUIRED_MODULES := \
  com.android.Trunks.conf \
  trunksd-seccomp.policy \

LOCAL_SRC_FILES := \
  resource_manager.cc \
  tpm_handle.cc \
  tpm_simulator_handle.cc \
  trunks_binder_service.cc \
  trunksd.cc \

include $(BUILD_EXECUTABLE)

# trunksd-seccomp.policy
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := trunksd-seccomp.policy
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/share/policy/
LOCAL_SRC_FILES := trunksd-seccomp-$(TARGET_ARCH).policy
include $(BUILD_PREBUILT)

# libtrunks
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libtrunks
LOCAL_CPP_EXTENSION := $(trunksCppExtension)
LOCAL_CFLAGS := $(trunksCFlags)
LOCAL_CLANG := true
LOCAL_C_INCLUDES := $(trunksIncludes)
LOCAL_SHARED_LIBRARIES := $(trunksSharedLibraries)
LOCAL_STATIC_LIBRARIES := \
  libtrunks_common \
  libtrunks_generated \

LOCAL_SRC_FILES := \
  trunks_binder_proxy.cc \

include $(BUILD_SHARED_LIBRARY)

# trunks_client
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := trunks_client
LOCAL_CPP_EXTENSION := $(trunksCppExtension)
LOCAL_CFLAGS := $(trunksCFlags)
LOCAL_CLANG := true
LOCAL_C_INCLUDES := $(trunksIncludes)
LOCAL_SHARED_LIBRARIES := $(trunksSharedLibraries) libtrunks
LOCAL_STATIC_LIBRARIES := libtrunks_common
LOCAL_SRC_FILES := \
  trunks_client.cc \
  trunks_client_test.cc \

include $(BUILD_EXECUTABLE)

# Target unit tests
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := trunks_test
LOCAL_MODULE_TAGS := eng
LOCAL_CPP_EXTENSION := $(trunksCppExtension)
LOCAL_CFLAGS := $(trunksCFlags)
LOCAL_CLANG := true
LOCAL_C_INCLUDES := $(trunksIncludes)
LOCAL_SHARED_LIBRARIES := $(trunksSharedLibraries)
LOCAL_SRC_FILES := \
  background_command_transceiver_test.cc \
  hmac_authorization_delegate_test.cc \
  hmac_session_test.cc \
  mock_authorization_delegate.cc \
  mock_blob_parser.cc \
  mock_command_transceiver.cc \
  mock_hmac_session.cc \
  mock_policy_session.cc \
  mock_session_manager.cc \
  mock_tpm.cc \
  mock_tpm_state.cc \
  mock_tpm_utility.cc \
  password_authorization_delegate_test.cc \
  policy_session_test.cc \
  resource_manager.cc \
  resource_manager_test.cc \
  scoped_key_handle_test.cc \
  session_manager_test.cc \
  tpm_generated_test.cc \
  tpm_state_test.cc \
  tpm_utility_test.cc \
  trunks_factory_for_test.cc \

LOCAL_STATIC_LIBRARIES := \
  libgmock \
  libgtest \
  libBionicGtestMain \
  libtrunks_common \
  libtrunks_generated \

include $(BUILD_NATIVE_TEST)
