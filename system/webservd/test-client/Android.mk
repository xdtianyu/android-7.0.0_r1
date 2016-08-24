#
# Copyright 2016 The Android Open Source Project
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

# executable run in tests to validate that the webserver is
# working correctly
include $(CLEAR_VARS)
LOCAL_MODULE := webservd_test_client

# We want this executable installed for testing purposes, but only on brillo
# images.
ifdef BRILLO
  LOCAL_MODULE_TAGS := debug
endif

LOCAL_INIT_RC := webservd_test_client.rc
LOCAL_SRC_FILES := \
    main.cc

# Contrary to our own instructions, we're not going to include this last.
# We're going define our own libraries and include paths as if we were actually
# a client.
$(eval $(webservd_common))

LOCAL_C_INCLUDES :=
LOCAL_SHARED_LIBRARIES := \
    libbrillo \
    libchrome \
    libwebserv

ifeq ($(system_webservd_use_dbus),true)
LOCAL_SHARED_LIBRARIES += \
    libdbus \
    libbrillo-dbus \
    libchrome-dbus
endif

ifeq ($(system_webservd_use_binder),true)
LOCAL_SHARED_LIBRARIES += \
    libbinder \
    libbrillo-binder
endif

include $(BUILD_EXECUTABLE)
