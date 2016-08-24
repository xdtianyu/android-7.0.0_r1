LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CLANG := true

LOCAL_SRC_FILES := ss.c ssfilter.y

LOCAL_MODULE := ss

LOCAL_MODULE_TAGS := debug

LOCAL_SHARED_LIBRARIES += libiprouteutil libnetlink

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include

##
# "-x c" forces the lex/yacc files to be compiled as c the build system
# otherwise forces them to be c++.
yacc_flags := -x c

LOCAL_CFLAGS := \
    -O2 -g \
    -W -Wall \
    -Wno-missing-field-initializers \
    -Wno-sign-compare \
    -Wno-tautological-pointer-compare \
    -Wno-unused-parameter \
    -Werror \
    '-Dsethostent(x)=' \
    $(yacc_flags) \
    -DHAVE_SETNS

LOCAL_CPPFLAGS := $(yacc_flags)

LOCAL_LDFLAGS := -Wl,-export-dynamic
include $(BUILD_EXECUTABLE)

