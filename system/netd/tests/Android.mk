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

# APCT build target
include $(CLEAR_VARS)
LOCAL_MODULE := netd_integration_test
LOCAL_CFLAGS := -Wall -Werror -Wunused-parameter
EXTRA_LDLIBS := -lpthread
LOCAL_SHARED_LIBRARIES += libbase libbinder libcutils liblog liblogwrap libnetdaidl libnetd_client \
                          libutils
LOCAL_STATIC_LIBRARIES += libtestUtil
LOCAL_AIDL_INCLUDES := system/netd/server/binder
LOCAL_C_INCLUDES += system/netd/include system/extras/tests/include system/netd/binder/include \
                    system/netd/server system/core/logwrapper/include \
                    system/extras/tests/include bionic/libc/dns/include
# netd_integration_test.cpp is currently empty and exists only so that we can do:
# runtest -x system/netd/tests/netd_integration_test.cpp
LOCAL_SRC_FILES := binder_test.cpp \
                   dns_responder.cpp \
                   netd_integration_test.cpp \
                   netd_test.cpp \
                   ../server/NetdConstants.cpp
LOCAL_MODULE_TAGS := eng tests
include $(BUILD_NATIVE_TEST)
