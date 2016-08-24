# Copyright (c) 2016, Intel Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

ifneq ($(USE_CUSTOM_PARAMETER_FRAMEWORK), true)

#
# Do not allow to use the networking feature through socket (debug purpose of the PFW)
# for user build.
#
ifeq ($(TARGET_BUILD_VARIANT),user)
PFW_NETWORKING := false
PFW_NETWORKING_SUFFIX := -no-networking
endif

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/LibPfwUtility.mk
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE := true
include $(LOCAL_PATH)/LibPfwUtility.mk
include $(BUILD_HOST_STATIC_LIBRARY)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/LibRemoteProcessor.mk
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE := true
include $(LOCAL_PATH)/LibRemoteProcessor.mk
LOCAL_LDLIBS += -lpthread
include $(BUILD_HOST_SHARED_LIBRARY)

# build libparameter
include $(CLEAR_VARS)
include $(LOCAL_PATH)/LibParameter.mk
LOCAL_SHARED_LIBRARIES += libicuuc libdl
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE := true
include $(LOCAL_PATH)/LibParameter.mk
LOCAL_SHARED_LIBRARIES += libicuuc-host
LOCAL_LDLIBS := -ldl
include $(BUILD_HOST_SHARED_LIBRARY)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/TestPlatform.mk
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE := true
include $(LOCAL_PATH)/TestPlatform.mk
LOCAL_LDLIBS := -lpthread
include $(BUILD_HOST_EXECUTABLE)

ifneq ($(PFW_NETWORKING),false)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/RemoteProcess.mk
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE := true
include $(LOCAL_PATH)/RemoteProcess.mk
include $(BUILD_HOST_EXECUTABLE)

endif #ifneq ($(PFW_NETWORKING),false)

include $(LOCAL_PATH)/XmlGenerator.mk
include $(LOCAL_PATH)/Schemas.mk

endif #ifneq ($(USE_CUSTOM_PARAMETER_FRAMEWORK), true)
