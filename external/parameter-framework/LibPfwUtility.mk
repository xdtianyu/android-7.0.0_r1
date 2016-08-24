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

LOCAL_MODULE := libpfw_utility$(SUFFIX)
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_SRC_FILES := \
    upstream/utility/DynamicLibrary.cpp \
    upstream/utility/posix/DynamicLibrary.cpp \
    upstream/utility/Tokenizer.cpp \
    upstream/utility/Utility.cpp

target_copy_headers := \
    upstream/utility/NonCopyable.hpp \
    upstream/utility/ErrorContext.hpp \
    upstream/utility/Utility.h \
    upstream/utility/convert.hpp

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/upstream/utility/
LOCAL_C_INCLUDES := $(LOCAL_PATH)/upstream/utility/
LOCAL_CFLAGS := -frtti -fexceptions

LOCAL_COPY_HEADERS := $(target_copy_headers)

LOCAL_CLANG := true
