LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE:=avahi-browse

LOCAL_SRC_FILES := \
    avahi-browse.c \
    sigint.c

LOCAL_SHARED_LIBRARIES:=\
    libavahi-client \
    libavahi-common \
    libdbus

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
    -DDEBUG_TRAP=__asm__\(\"int\ $3\"\)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    external/avahi

include $(BUILD_EXECUTABLE)
