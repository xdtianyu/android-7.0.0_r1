# Copyright (C) 2014 The Android Open Source Project
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

ifeq ($(HOST_OS), linux)
include $(CLEAR_VARS)

ifeq ($(vg_build_second_arch),true)
  LOCAL_MULTILIB := 32
  vg_local_arch := x86
else
  LOCAL_MULTILIB := first
  vg_local_arch := amd64
endif

# TODO: This workaround is to avoid calling memset from VG(memset)
# wrapper because of invalid clang optimization; This seems to be
# limited to amd64/x86 codegen(?);
LOCAL_CLANG := false

LOCAL_MODULE := $(vg_local_module)-$(vg_local_arch)-linux

LOCAL_SRC_FILES := $(vg_local_src_files)

LOCAL_C_INCLUDES := $(common_includes) $(vg_local_c_includes)

LOCAL_CFLAGS := $(vg_local_cflags) $(vg_local_host_arch_cflags)

LOCAL_ASFLAGS := $(common_cflags) $(vg_local_host_arch_cflags)

LOCAL_LDFLAGS := $(vg_local_ldflags) -lpthread

LOCAL_MODULE_CLASS := $(vg_local_module_class)

LOCAL_STATIC_LIBRARIES := \
    $(foreach l,$(vg_local_static_libraries),$l-$(vg_local_arch)-linux)
LOCAL_WHOLE_STATIC_LIBRARIES := \
    $(foreach l,$(vg_local_whole_static_libraries),$l-$(vg_local_arch)-linux)

ifeq ($(vg_local_target),EXECUTABLE)
  LOCAL_FORCE_STATIC_EXECUTABLE := true
  LOCAL_NO_FPIE := true
endif

ifneq ($(vg_local_target),STATIC_LIBRARY)
  LOCAL_MODULE_PATH=$(HOST_OUT)/lib64/valgrind
endif

ifeq ($(vg_local_without_system_shared_libraries),true)
  LOCAL_SYSTEM_SHARED_LIBRARIES :=
  # for host 32 bit we need -static-libgcc
  LOCAL_LDFLAGS += -lgcc
endif

ifeq ($(vg_local_no_crt),true)
  LOCAL_NO_CRT := true
endif

LOCAL_CXX_STL := none
LOCAL_SANITIZE := never

include $(BUILD_HOST_$(vg_local_target))

endif
