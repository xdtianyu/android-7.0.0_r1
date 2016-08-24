LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE:=libavahi-common

LOCAL_SRC_FILES := \
    avahi-malloc.c \
    address.c \
    alternative.c \
    error.c \
    strlst.c \
    domain.c \
    timeval.c \
    simple-watch.c \
    thread-watch.c \
    rlist.c \
    utf8.c \
    i18n.c

LOCAL_CFLAGS := \
    -DHAVE_CONFIG_H \
    -g \
    -O2 \
    -fstack-protector \
    -std=c99 \
    -Wall \
    -W \
    -Wextra \
    -pedantic \
    -pipe \
    -Wformat \
    -Wold-style-definition \
    -Wdeclaration-after-statement \
    -Wfloat-equal \
    -Wmissing-declarations \
    -Wmissing-prototypes \
    -Wstrict-prototypes \
    -Wredundant-decls \
    -Wmissing-noreturn \
    -Wshadow \
    -Wendif-labels \
    -Wpointer-arith \
    -Wbad-function-cast \
    -Wcast-qual \
    -Wcast-align \
    -Wwrite-strings \
    -fdiagnostics-show-option \
    -Wno-cast-qual \
    -fno-strict-aliasing \
    -DDEBUG_TRAP=__asm__\(\"int\ $3\"\) \
    -DAVAHI_LOCALEDIR=\"/usr/local/share/locale\"

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    external/avahi

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/..

include $(BUILD_SHARED_LIBRARY)
