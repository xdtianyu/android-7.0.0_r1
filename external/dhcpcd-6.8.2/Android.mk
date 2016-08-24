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
# dhcpcd-6.8.2 is D-Bus enabled and compatible with Brillo daemons. dhcpcd
# is the standard version of this daemon used in Android.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := dhcpcd-6.8.2
LOCAL_SRC_FILES := \
    common.c \
    control.c \
    dhcpcd.c \
    duid.c \
    eloop.c \
    if.c \
    if-options.c \
    dhcp-common.c \
    auth.c \
    if-linux.c \
    if-linux-wext.c \
    arp.c \
    dhcp.c \
    ipv4.c \
    ipv4ll.c \
    ifaddrs.c \
    crypt/md5.c \
    crypt/hmac_md5.c \
    compat/closefrom.c \
    compat/strtoi.c \
    dhcpcd-embedded.c \
    compat/posix_spawn.c

# Always support IPv4.
LOCAL_CFLAGS += -DINET

ifeq ($(DHCPCD_USE_SCRIPT), yes)
LOCAL_SRC_FILES += script.c
else
LOCAL_SRC_FILES += script-stub.c
endif

ifeq ($(DHCPCD_USE_IPV6), yes)
LOCAL_SRC_FILES += ipv6.c ipv6nd.c dhcp6.c
LOCAL_SRC_FILES += crypt/sha256.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/crypt
LOCAL_CFLAGS += -DINET6
endif

ifeq ($(DHCPCD_USE_DBUS), yes)
LOCAL_SRC_FILES += dbus/dbus-dict.c dbus/rpc-dbus.c
LOCAL_CFLAGS += -DCONFIG_DBUS -DPASSIVE_MODE
LOCAL_SHARED_LIBRARIES += libdbus
else
LOCAL_SRC_FILES += rpc-stub.c
endif

# Compiler complains about implicit function delcarations in dhcpcd.c,
# if-options.c, dhcp-common.c, and crypt/sha256.c.
LOCAL_CFLAGS += -Wno-implicit-function-declaration
# Compiler complains about signed-unsigned comparison in dhcp-common.c.
LOCAL_CFLAGS += -Wno-sign-compare
# Compiler complains about unused parameters in function stubs in rpc-stub.c.
LOCAL_CFLAGS += -Wno-unused-parameter
# Compiler complains about possibly uninitialized variables in rpc-dbus.c.
LOCAL_CFLAGS += -Wno-maybe-uninitialized

LOCAL_SHARED_LIBRARIES += libc libcutils libnetutils
include $(BUILD_EXECUTABLE)

# Each build target using dhcpcd-6.8.2 should define its own source
# and destination for its dhcpcd.conf file.
# include $(CLEAR_VARS)
# LOCAL_MODULE := dhcpcd-6.8.2.conf
# LOCAL_MODULE_CLASS := ETC
# LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/dhcpcd-6.8.2
# LOCAL_SRC_FILES := android.conf
# include $(BUILD_PREBUILT)
