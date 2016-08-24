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

ifeq ($(LOCAL_IS_HOST_MODULE),true)
SUFFIX := _host
else
SUFFIX :=
endif

LOCAL_MODULE := test-platform$(SUFFIX)
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := \
    upstream/test/test-platform/main.cpp \
    upstream/test/test-platform/TestPlatform.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/upstream/test/test-platform

LOCAL_STATIC_LIBRARIES := libpfw_utility$(SUFFIX)
LOCAL_SHARED_LIBRARIES := \
    libparameter$(SUFFIX) \
    libremote-processor$(PFW_NETWORKING_SUFFIX)$(SUFFIX)

ifeq ($(PFW_NETWORKING),false)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/upstream/asio/stub
LOCAL_CFLAGS := -frtti -fexceptions

else

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/support/android/asio

LOCAL_CFLAGS :=  \
    -frtti -fexceptions \
    -isystem $(LOCAL_PATH)/asio/include

endif #ifeq ($(PFW_NETWORKING),false)

LOCAL_CLANG := true
