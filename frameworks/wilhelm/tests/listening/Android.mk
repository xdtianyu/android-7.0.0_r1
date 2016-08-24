LOCAL_PATH:= $(call my-dir)

#

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	seekTorture.c

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_MODULE:= slesTest_seekTorture

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_EXECUTABLE)

#

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	slesTest_playMuteSolo.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_MODULE:= slesTest_playMuteSolo

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_EXECUTABLE)
