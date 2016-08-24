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

libevent_linux_src := \
	epoll.c \
	poll.c \
	select.c \

libevent_darwin_src := \
	kqueue.c \
	poll.c \
	select.c \

libevent_core_src := \
	buffer.c \
	bufferevent.c \
	bufferevent_filter.c \
	bufferevent_pair.c \
	bufferevent_ratelim.c \
	bufferevent_sock.c \
	event.c \
	evmap.c \
	evthread.c \
	evutil.c \
	evutil_rand.c \
	listener.c \
	log.c \
	signal.c \
	strlcpy.c

libevent_extra_src := \
	evdns.c \
	event_tagging.c \
	evrpc.c \
	http.c

libevent_all_src := \
	$(libevent_core_src) \
	$(libevent_extra_src)

libevent_cflags := \
	-O3 \
	-Wno-implicit-function-declaration \
	-Wno-strict-aliasing \
	-Wno-unused-parameter \
	-Werror

include $(CLEAR_VARS)
LOCAL_MODULE := libevent
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := $(libevent_cflags)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_SHARED_LIBRARIES := libc
LOCAL_SRC_FILES := $(libevent_all_src) $(libevent_linux_src)
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libevent-host
LOCAL_CFLAGS := $(libevent_cflags)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_SRC_FILES := $(libevent_all_src) $(libevent_$(HOST_OS)_src)
include $(BUILD_HOST_SHARED_LIBRARY)
