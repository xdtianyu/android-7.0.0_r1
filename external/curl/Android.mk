# Google Android makefile for curl and libcurl
#
# This file is an updated version of Dan Fandrich's Android.mk, meant to build
# curl for ToT android with the android build system.

LOCAL_PATH:= $(call my-dir)

# Curl needs a version string.
# As this will be compiled on multiple platforms, generate a version string from
# the build environment variables.
version_string := "Android $(PLATFORM_VERSION) $(TARGET_ARCH_VARIANT)"

curl_CFLAGS := -Wpointer-arith -Wwrite-strings -Wunused -Winline \
	-Wnested-externs -Wmissing-declarations -Wmissing-prototypes -Wno-long-long \
	-Wfloat-equal -Wno-multichar -Wno-sign-compare -Wno-format-nonliteral \
	-Wendif-labels -Wstrict-prototypes -Wdeclaration-after-statement \
	-Wno-system-headers -DHAVE_CONFIG_H -DOS='$(version_string)' -Werror

curl_includes := \
	$(LOCAL_PATH)/include/ \
	$(LOCAL_PATH)/lib

#########################
# Build the libcurl static library

include $(CLEAR_VARS)
include $(LOCAL_PATH)/lib/Makefile.inc

LOCAL_SRC_FILES := $(addprefix lib/,$(CSOURCES))
LOCAL_C_INCLUDES := $(curl_includes)
LOCAL_CFLAGS := $(curl_CFLAGS)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_MODULE:= libcurl
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES := libcrypto_static libz

include $(BUILD_STATIC_LIBRARY)

#########################
# Build the libcurl shared library

libcurl_shared_libs := libcrypto libssl libz

include $(CLEAR_VARS)
include $(LOCAL_PATH)/lib/Makefile.inc

LOCAL_SRC_FILES := $(addprefix lib/,$(CSOURCES))
LOCAL_C_INCLUDES := $(curl_includes)
LOCAL_CFLAGS := \
    $(curl_CFLAGS) \
    -D_GNU_SOURCE=1
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_MODULE:= libcurl-host
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(libcurl_shared_libs:=-host)

include $(BUILD_HOST_SHARED_LIBRARY)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/lib/Makefile.inc

LOCAL_SRC_FILES := $(addprefix lib/,$(CSOURCES))
LOCAL_C_INCLUDES := $(curl_includes)
LOCAL_CFLAGS := $(curl_CFLAGS)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_MODULE:= libcurl
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(libcurl_shared_libs)

include $(BUILD_SHARED_LIBRARY)

#########################
# Build the curl binary

include $(CLEAR_VARS)
include $(LOCAL_PATH)/src/Makefile.inc
LOCAL_SRC_FILES := $(addprefix src/,$(CURL_CFILES))

LOCAL_MODULE := curl
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES := libcurl
LOCAL_SHARED_LIBRARIES := libcrypto libssl libz

LOCAL_C_INCLUDES := $(curl_includes)

# This may also need to include $(CURLX_CFILES) in order to correctly link
# if libcurl is changed to be built as a dynamic library
LOCAL_CFLAGS := $(curl_CFLAGS)

include $(BUILD_EXECUTABLE)
