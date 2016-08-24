LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE:=libavahi-core

LOCAL_SRC_FILES := \
    timeeventq.c \
    iface.c \
    server.c \
    entry.c \
    prioq.c \
    cache.c \
    socket.c \
    response-sched.c \
    query-sched.c \
    probe-sched.c \
    announce.c \
    browse.c \
    rrlist.c \
    resolve-host-name.c \
    resolve-address.c \
    browse-domain.c \
    browse-service-type.c \
    browse-service.c \
    resolve-service.c \
    dns.c \
    rr.c \
    log.c \
    browse-dns-server.c \
    fdutil.c \
    util.c \
    hashmap.c \
    wide-area.c \
    multicast-lookup.c \
    querier.c \
    addr-util.c \
    domain-util.c \
    iface-linux.c \
    netlink.c

LOCAL_SHARED_LIBRARIES:=\
    libavahi-common \
    liblog

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

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/..

include $(BUILD_SHARED_LIBRARY)
