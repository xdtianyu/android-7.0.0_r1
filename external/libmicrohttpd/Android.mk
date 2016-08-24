#
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
LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libmicrohttpd
LOCAL_CFLAGS := -Wno-sign-compare -Wno-unused-parameter
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/src/include

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/src/include \
    $(LOCAL_PATH)/src/microhttpd \
    external/boringssl/include \

LOCAL_SHARED_LIBRARIES := libssl libcrypto

LOCAL_SRC_FILES := \
    src/microhttpd/base64.c \
    src/microhttpd/basicauth.c \
    src/microhttpd/connection.c \
    src/microhttpd/connection_https.c \
    src/microhttpd/daemon.c \
    src/microhttpd/digestauth.c \
    src/microhttpd/internal.c \
    src/microhttpd/md5.c \
    src/microhttpd/memorypool.c \
    src/microhttpd/postprocessor.c \
    src/microhttpd/reason_phrase.c \
    src/microhttpd/response.c \
    src/microhttpd/tsearch.c \

include $(BUILD_SHARED_LIBRARY)
