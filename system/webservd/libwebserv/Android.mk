#
# Copyright 2015 The Android Open Source Project
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

# libwebserv shared library
# ========================================================

include $(CLEAR_VARS)
LOCAL_MODULE := libwebserv
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/..
LOCAL_SHARED_LIBRARIES :=
LOCAL_SRC_FILES := \
    dbus_bindings/org.chromium.WebServer.RequestHandler.dbus-xml \
    protocol_handler.cc \
    request.cc \
    request_handler_callback.cc \
    request_utils.cc \
    response.cc \
    server.cc \

ifeq ($(system_webservd_use_dbus),true)
LOCAL_SHARED_LIBRARIES += libwebservd-client-internal
LOCAL_SRC_FILES += \
    dbus_protocol_handler.cc \
    dbus_server.cc
endif

ifeq ($(system_webservd_use_binder),true)
LOCAL_SHARED_LIBRARIES += libwebserv-binder-internal
LOCAL_SRC_FILES += \
    binder_server.cc
endif

$(eval $(webservd_common))
$(eval $(webservd_common_libraries))
include $(BUILD_SHARED_LIBRARY)

# libwebserv-proxies-internal shared library
# ========================================================
# You do not want to depend on this.  Depend on libwebserv instead.
# libwebserv abstracts and helps you consume this interface.
#
# This library builds the proxies which webservd will use to communicate back
# to libwebservd over DBus.
ifeq ($(system_webservd_use_dbus),true)
include $(CLEAR_VARS)
LOCAL_MODULE := libwebserv-proxies-internal

LOCAL_SRC_FILES := \
    dbus_bindings/org.chromium.WebServer.RequestHandler.dbus-xml \

LOCAL_DBUS_PROXY_PREFIX := libwebserv

$(eval $(webservd_common))
$(eval $(webservd_common_libraries))
include $(BUILD_SHARED_LIBRARY)
endif


# libwebserv-binder-internal shared library
# ========================================================
# You do not want to depend on this.  Depend on libwebserv instead.
# libwebserv abstracts and helps you consume this interface.
#
# This library builds the binder interfaces used between webservd and libwebserv
ifeq ($(system_webservd_use_binder),true)
include $(CLEAR_VARS)
LOCAL_MODULE := libwebserv-binder-internal

LOCAL_SRC_FILES += \
    ../aidl/android/webservd/IServer.aidl \
    ../aidl/android/webservd/IProtocolHandler.aidl \
    binder_constants.cc

LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/../aidl
$(eval $(webservd_common))
$(eval $(webservd_common_libraries))
include $(BUILD_SHARED_LIBRARY)
endif
