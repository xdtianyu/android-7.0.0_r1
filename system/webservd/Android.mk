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

webservd_root := $(my-dir)

system_webservd_use_dbus := true
system_webservd_use_binder :=

# Definitions applying to all targets. $(eval) this last.
define webservd_common
  LOCAL_CPP_EXTENSION := .cc
  LOCAL_CFLAGS += -Wall -Werror
  ifeq ($(system_webservd_use_dbus),true)
    LOCAL_CFLAGS += -DWEBSERV_USE_DBUS
  endif
  ifeq ($(system_webservd_use_binder),true)
    LOCAL_CFLAGS += -DWEBSERV_USE_BINDER
  endif

  # libbrillo's secure_blob.h calls "using Blob::vector" to expose its base
  # class's constructors. This causes a "conflicts with version inherited from
  # 'std::__1::vector<unsigned char>'" error when building with GCC.
  LOCAL_CLANG := true

  LOCAL_C_INCLUDES += \
    $(webservd_root) \
    external/gtest/include \

endef  # webserv_common

define webservd_common_libraries
  LOCAL_SHARED_LIBRARIES += \
      libbrillo \
      libbrillo-http \
      libbrillo-stream \
      libchrome \
      libmicrohttpd

  # TODO(wiley) Uncomment these guards once firewalld moves to binder
  #             b/25932807
  # ifeq ($(system_webservd_use_dbus),true)
    LOCAL_SHARED_LIBRARIES += \
        libbrillo-dbus \
        libchrome-dbus \
        libdbus
  # endif
  ifeq ($(system_webservd_use_binder),true)
    LOCAL_SHARED_LIBRARIES += \
        libbrillo-binder \
        libcutils \
        libutils \
        libbinder
  endif

endef  # webserv_common_libraries

include $(call all-subdir-makefiles)
