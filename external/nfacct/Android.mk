LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/android.config

INCLUDES = $(LOCAL_PATH)
INCLUDES += external/libnl/include

ifdef HAVE_LIBNL20
LOCAL_CFLAGS += -DHAVE_LIBNL20
endif

########################

LOCAL_SRC_FILES:= nfacct.c
LOCAL_MODULE := nfacct

LOCAL_SHARED_LIBRARIES += libnl
LOCAL_C_INCLUDES := $(INCLUDES)

include $(BUILD_EXECUTABLE)
