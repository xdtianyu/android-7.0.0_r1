LOCAL_PATH := $(call my-dir)

dbusToolsCommonCIncludes := $(LOCAL_PATH)/..
dbusToolsCommonCFlags := \
	-DDBUS_COMPILATION \
	-DDBUS_MACHINE_UUID_FILE=\"/etc/machine-id\" \
	-Wno-unused-parameter
dbusToolsCommonSharedLibraries := libdbus

# common

include $(CLEAR_VARS)

LOCAL_SRC_FILES := dbus-print-message.c
LOCAL_C_INCLUDES += $(dbusToolsCommonCIncludes)
LOCAL_SHARED_LIBRARIES += $(dbusToolsCommonSharedLibraries)
LOCAL_CFLAGS += $(dbusToolsCommonCFlags)
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE := libdbus-tools-common
include $(BUILD_STATIC_LIBRARY)

# dbus-monitor

include $(CLEAR_VARS)

LOCAL_SRC_FILES := dbus-monitor.c
LOCAL_C_INCLUDES += $(dbusToolsCommonCIncludes)
LOCAL_SHARED_LIBRARIES += $(dbusToolsCommonSharedLibraries)
LOCAL_STATIC_LIBRARIES += libdbus-tools-common
LOCAL_CFLAGS += $(dbusToolsCommonCFlags)
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE := dbus-monitor
include $(BUILD_EXECUTABLE)

# dbus-send

include $(CLEAR_VARS)

LOCAL_SRC_FILES := dbus-send.c
LOCAL_C_INCLUDES += $(dbusToolsCommonCIncludes)
LOCAL_SHARED_LIBRARIES += $(dbusToolsCommonSharedLibraries)
LOCAL_STATIC_LIBRARIES += libdbus-tools-common
LOCAL_CFLAGS += $(dbusToolsCommonCFlags)
LOCAL_MODULE := dbus-send
include $(BUILD_EXECUTABLE)
