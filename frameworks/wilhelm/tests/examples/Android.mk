LOCAL_PATH:= $(call my-dir)

# slesTest_recBuffQueue

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm) \
	$(call include-path-for, audio-utils)

LOCAL_SRC_FILES:= \
	slesTestRecBuffQueue.cpp

LOCAL_SHARED_LIBRARIES := \
	libaudioutils \
	libOpenSLES

LOCAL_STATIC_LIBRARIES := \
	libsndfile

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_CFLAGS := -Werror -Wall

LOCAL_MODULE:= slesTest_recBuffQueue

include $(BUILD_EXECUTABLE)

# slesTest_playFdPath

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	slesTestPlayFdPath.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_MODULE:= slesTest_playFdPath

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_EXECUTABLE)

# slesTest_feedback

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm) \
	$(call include-path-for, audio-utils)

LOCAL_SRC_FILES:= \
    slesTestFeedback.cpp

LOCAL_SHARED_LIBRARIES := \
	libaudioutils \
	libOpenSLES

LOCAL_STATIC_LIBRARIES := \
	libsndfile

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
	#LOCAL_SHARED_LIBRARIES += librt
endif

LOCAL_CFLAGS := -Werror -Wall
LOCAL_CFLAGS += -UNDEBUG

LOCAL_MODULE:= slesTest_feedback

include $(BUILD_EXECUTABLE)

# slesTest_sawtoothBufferQueue

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	slesTestSawtoothBufferQueue.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
	#LOCAL_SHARED_LIBRARIES += librt
endif

LOCAL_MODULE:= slesTest_sawtoothBufferQueue

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_EXECUTABLE)

# slesTest_eqFdPath

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	slesTestEqFdPath.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_MODULE:= slesTest_eqFdPath

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_EXECUTABLE)

# slesTest_eqOutputPath

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	slesTestEqOutputPath.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_MODULE:= slesTest_eqOutputPath

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_EXECUTABLE)

# slesTest_bassboostPath

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	slesTestBassBoostPath.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_MODULE:= slesTest_bassboostPath

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_EXECUTABLE)

# slesTest_virtualizer

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	slesTestVirtualizerPath.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_MODULE:= slesTest_virtualizer

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_EXECUTABLE)

# slesTest_effectCapabilities

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	slesTestEffectCapabilities.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_MODULE:= slesTest_effectCapabilities

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_EXECUTABLE)

# slesTest_sendToPresetReverb

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	slesTestSendToPresetReverb.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_MODULE:= slesTest_sendToPresetReverb

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_EXECUTABLE)

# slesTest_decodeToBuffQueue

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	slesTestDecodeToBuffQueue.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_MODULE:= slesTest_decodeToBuffQueue

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_EXECUTABLE)

###################
# slesTestDecodeAac

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	slesTestDecodeAac.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES

LOCAL_STATIC_LIBRARIES := libcpustats

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_CFLAGS := -Werror -Wall
LOCAL_CFLAGS += -UNDEBUG

LOCAL_MODULE:= slesTest_decodeAac

include $(BUILD_EXECUTABLE)

#######################################
# OpenMAX AL example code

# xaVideoDecoderCapabilities

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
	$(call include-path-for, wilhelm)

LOCAL_SRC_FILES:= \
	xaVideoDecoderCapabilities.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenMAXAL

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_MODULE:= xaVideoDecoderCapabilities

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_EXECUTABLE)
