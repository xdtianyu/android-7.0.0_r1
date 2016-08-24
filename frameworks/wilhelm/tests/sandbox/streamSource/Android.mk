LOCAL_PATH:= $(call my-dir)

# slesTest_playStream

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	slesTestPlayStream.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES

LOCAL_CFLAGS := -Werror -Wall
ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_MODULE:= slesTest_playStream


include $(BUILD_EXECUTABLE)
