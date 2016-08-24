LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=clatd.c dump.c checksum.c translate.c icmp.c ipv4.c ipv6.c config.c dns64.c logging.c getaddr.c netlink_callbacks.c netlink_msg.c setif.c mtu.c tun.c ring.c

LOCAL_CFLAGS := -Wall -Werror -Wunused-parameter
LOCAL_C_INCLUDES := external/libnl/include bionic/libc/dns/include
LOCAL_STATIC_LIBRARIES := libnl
LOCAL_SHARED_LIBRARIES := libcutils liblog libnetutils

# The clat daemon.
LOCAL_MODULE := clatd

include $(BUILD_EXECUTABLE)


# The configuration file.
include $(CLEAR_VARS)

LOCAL_MODULE := clatd.conf
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc
LOCAL_SRC_FILES := $(LOCAL_MODULE)

include $(BUILD_PREBUILT)


# Unit tests.
include $(CLEAR_VARS)

LOCAL_MODULE := clatd_test
LOCAL_CFLAGS := -Wall -Werror -Wunused-parameter
LOCAL_SRC_FILES := clatd_test.cpp checksum.c translate.c icmp.c ipv4.c ipv6.c logging.c config.c tun.c
LOCAL_MODULE_TAGS := eng tests
LOCAL_SHARED_LIBRARIES := liblog

include $(BUILD_NATIVE_TEST)

# Microbenchmark.
include $(CLEAR_VARS)

LOCAL_CLANG := true
LOCAL_MODULE := clatd_microbenchmark
LOCAL_CFLAGS := -Wall -Werror -Wunused-parameter
LOCAL_SRC_FILES := clatd_microbenchmark.c checksum.c tun.c
LOCAL_MODULE_TAGS := eng tests

include $(BUILD_NATIVE_TEST)
