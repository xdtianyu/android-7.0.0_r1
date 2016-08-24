#
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
#

# If you actually want to use ltrace, let android-bionic@ know.
# One of its dependencies (libelf) won't build with clang,
# and we want to know whether anyone actually cares...
ifeq (true,false)

LOCAL_PATH := $(call my-dir)

# -------------------------------------------------------------------------

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    bits.c \
    breakpoints.c \
    debug.c \
    demangle.c \
    dict.c \
    execute_program.c \
    expr.c \
    fetch.c \
    filter.c \
    glob.c \
    handle_event.c \
    lens.c \
    lens_default.c \
    lens_enum.c \
    libltrace.c \
    library.c \
    ltrace-elf.c \
    main.c \
    memstream.c \
    options.c \
    output.c \
    param.c \
    printf.c \
    proc.c \
    prototype.c \
    read_config_file.c \
    summary.c \
    type.c \
    value.c \
    value_dict.c \
    vect.c \
    zero.c \
    sysdeps/linux-gnu/breakpoint.c \
    sysdeps/linux-gnu/events.c \
    sysdeps/linux-gnu/hooks.c \
    sysdeps/linux-gnu/proc.c \
    sysdeps/linux-gnu/trace.c \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/sysdeps/ \
    $(LOCAL_PATH)/sysdeps/linux-gnu \

arm_path := sysdeps/linux-gnu/arm
LOCAL_C_INCLUDES_arm := $(LOCAL_PATH)/$(arm_path)
LOCAL_SRC_FILES_arm := \
    $(arm_path)/breakpoint.c \
    $(arm_path)/fetch.c \
    $(arm_path)/plt.c \
    $(arm_path)/regs.c \
    $(arm_path)/trace.c \

arm64_path := sysdeps/linux-gnu/aarch64
LOCAL_C_INCLUDES_arm64 := $(LOCAL_PATH)/$(arm64_path)
LOCAL_SRC_FILES_arm64 := \
    $(arm64_path)/fetch.c \
    $(arm64_path)/plt.c \
    $(arm64_path)/regs.c \
    $(arm64_path)/trace.c \

mips_path := sysdeps/linux-gnu/mips
LOCAL_C_INCLUDES_mips := $(LOCAL_PATH)/$(mips_path)
LOCAL_SRC_FILES_mips := \
    $(mips_path)/plt.c \
    $(mips_path)/regs.c \
    $(mips_path)/trace.c \

x86_path := sysdeps/linux-gnu/x86
LOCAL_C_INCLUDES_x86 := $(LOCAL_PATH)/$(x86_path)
LOCAL_SRC_FILES_x86 := \
    $(x86_path)/fetch.c \
    $(x86_path)/plt.c \
    $(x86_path)/regs.c \
    $(x86_path)/trace.c \

# x86_64 uses the same source as x86.
LOCAL_C_INCLUDES_x86_64 := $(LOCAL_C_INCLUDES_x86)
LOCAL_SRC_FILES_x86_64 := $(LOCAL_SRC_FILES_x86)

LOCAL_CFLAGS += \
    -DELF_HASH_TAKES_CHARP=1 \
    -DHAVE_ALARM=1 \
    -DHAVE_ATEXIT=1 \
    -DHAVE_DLFCN_H=1 \
    -DHAVE_ELF_C_READ_MMAP=1 \
    -DHAVE_ELF_H=1 \
    -DHAVE_FCNTL_H=1 \
    -DHAVE_FORK=1 \
    -DHAVE_GELF_H=1 \
    -DHAVE_GETOPT_LONG=1 \
    -DHAVE_GETTIMEOFDAY=1 \
    -DHAVE_INTTYPES_H=1 \
    -DHAVE_LIBELF=1 \
    -DHAVE_LIBSELINUX=1 \
    -DHAVE_LIBSTDC__=1 \
    -DHAVE_LIBUNWIND=1 \
    -DHAVE_LIBUNWIND_PTRACE=1 \
    -DHAVE_LIMITS_H=1 \
    -DHAVE_MEMORY_H=1 \
    -DHAVE_MEMSET=1 \
    -DHAVE_OPEN_MEMSTREAM=1 \
    -DHAVE_SELINUX_SELINUX_H=1 \
    -DHAVE_STDDEF_H=1 \
    -DHAVE_STDINT_H=1 \
    -DHAVE_STDLIB_H=1 \
    -DHAVE_STRCHR=1 \
    -DHAVE_STRDUP=1 \
    -DHAVE_STRERROR=1 \
    -DHAVE_STRINGS_H=1 \
    -DHAVE_STRING_H=1 \
    -DHAVE_STRSIGNAL=1 \
    -DHAVE_STRTOL=1 \
    -DHAVE_STRTOUL=1 \
    -DHAVE_SYS_IOCTL_H=1 \
    -DHAVE_SYS_PARAM_H=1 \
    -DHAVE_SYS_STAT_H=1 \
    -DHAVE_SYS_TIME_H=1 \
    -DHAVE_SYS_TYPES_H=1 \
    -DHAVE_UNISTD_H=1 \
    -DHAVE_UNWINDER=1 \
    -DHAVE_VFORK=1 \
    -DHAVE_WORKING_FORK=1 \
    -DHAVE_WORKING_VFORK=1 \
    -DLT_OBJDIR='".libs"' \
    -DPACKAGE='"ltrace"' \
    -DPACKAGE_BUGREPORT='"ltrace-devel@lists.alioth.debian.org"' \
    -DPACKAGE_NAME='"ltrace"' \
    -DPACKAGE_STRING='"ltrace 0.7.91"' \
    -DPACKAGE_TARNAME='"ltrace"' \
    -DPACKAGE_URL='"http://ltrace.alioth.debian.org/"' \
    -DPACKAGE_VERSION='"0.7.91"' \
    -DVERSION='"0.7.91"' \
    -D_FILE_OFFSET_BITS=64 \
    -D_LARGE_FILES=1 \
    -DPKGDATADIR=NULL \
    -DSYSCONFDIR='"/etc/"' \
    -Drindex=strrchr \

LOCAL_CFLAGS_32 += -DSIZEOF_LONG=4
LOCAL_CFLAGS_64 += -DSIZEOF_LONG=8

LOCAL_CFLAGS += \
    -Wall \
    -Wno-missing-field-initializers \
    -Wno-unused-parameter \
    -Wno-sign-compare \

LOCAL_STATIC_LIBRARIES := libelf

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libselinux \
    libunwind \

LOCAL_MODULE := ltrace

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE_TAGS := debug

LOCAL_MODULE_TARGET_ARCH := arm arm64 x86 x86_64

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

include $(BUILD_EXECUTABLE)

endif
