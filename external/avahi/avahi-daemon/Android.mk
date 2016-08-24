LOCAL_PATH:=$(call my-dir)

# Build avahi-daemon, install the conf file, and generate the init rc file.

include $(CLEAR_VARS)

LOCAL_MODULE := avahi-daemon

LOCAL_REQUIRED_MODULES := \
        avahi-daemon.conf \
        avahi-daemon.rc \

LOCAL_SRC_FILES := \
        main.c \
        static-services.c \
        static-hosts.c \
        ini-file-parser.c \
        setproctitle.c \
        sd-daemon.c \
        glob.c \
        ../avahi-client/check-nss.c \
        dbus-protocol.c \
        dbus-util.c \
        dbus-async-address-resolver.c \
        dbus-async-host-name-resolver.c \
        dbus-async-service-resolver.c \
        dbus-domain-browser.c \
        dbus-entry-group.c \
        dbus-service-browser.c \
        dbus-service-type-browser.c \
        dbus-sync-address-resolver.c \
        dbus-sync-host-name-resolver.c \
        dbus-sync-service-resolver.c \
        dbus-record-browser.c \
        ../avahi-common/dbus.c \
        ../avahi-common/dbus-watch-glue.c

LOCAL_SHARED_LIBRARIES:=\
        libavahi-common \
        libavahi-core \
        libexpat \
        libdl \
        libdaemon \
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
        -DDEBUG_TRAP=__asm__\(\"int\ $3\"\) \
        -DAVAHI_CONFIG_FILE=\"/system/etc/avahi-daemon.conf\" \
        -DAVAHI_HOSTS_FILE=\"/data/misc/avahi/hosts\" \
        -DAVAHI_SERVICE_DIR=\"/data/misc/avahi/services/\" \
        -DUSE_EXPAT_H \
        -DDBUS_VERSION_MAJOR=1 \
        -DDBUS_VERSION_MINOR=6 \
        -DDBUS_VERSION_MICRO=18 \
        -DDBUS_API_SUBJECT_TO_CHANGE \
        -DDBUS_SYSTEM_BUS_DEFAULT_ADDRESS=\"unix:path=/dev/socket/dbus\"

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH) \
        external/avahi

include $(BUILD_EXECUTABLE)

# /system/etc/avahi-daemon.conf
include $(CLEAR_VARS)
LOCAL_MODULE := avahi-daemon.conf
LOCAL_SRC_FILES := avahi-daemon.conf
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TARGET := $(TARGET_OUT_ETC)
include $(BUILD_PREBUILT)

# avahi-daemon.rc generation
ifdef INITRC_TEMPLATE
include $(CLEAR_VARS)
LOCAL_MODULE := avahi-daemon.rc
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/$(TARGET_COPY_OUT_INITRCD)

include $(BUILD_SYSTEM)/base_rules.mk

.PHONY: $(LOCAL_BUILT_MODULE)
$(LOCAL_BUILT_MODULE): $(INITRC_TEMPLATE)
	$(call generate-initrc-file,avahi-daemon,\
          --syslog --no-drop-root,inet)
endif
