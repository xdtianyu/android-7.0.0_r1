LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE:=libavahi-client

LOCAL_SRC_FILES := \
    client.c \
    entrygroup.c \
    browser.c \
    resolver.c \
    xdg-config.c \
    check-nss.c \
    ../avahi-common/dbus.c \
    ../avahi-common/dbus-watch-glue.c

LOCAL_SHARED_LIBRARIES:=\
    libdbus \
    libavahi-common \
    libdl

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
    -DDBUS_VERSION_MAJOR=1 \
    -DDBUS_VERSION_MINOR=6 \
    -DDBUS_VERSION_MICRO=18 \
    -DDBUS_API_SUBJECT_TO_CHANGE \
    -DDBUS_SYSTEM_BUS_DEFAULT_ADDRESS=\"unix:path=/var/run/dbus/system_bus_socket\"

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    external/avahi

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/..

include $(BUILD_SHARED_LIBRARY)
