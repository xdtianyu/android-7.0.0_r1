LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libaudioutils
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
	channels.c \
	conversion.cpp \
	fifo.c \
	fixedfft.cpp.arm \
	format.c \
	limiter.c \
	minifloat.c \
	primitives.c \
	resampler.c \
	roundup.c \
	echo_reference.c

LOCAL_C_INCLUDES += $(call include-path-for, speex)
LOCAL_C_INCLUDES += \
	$(call include-path-for, speex) \
	$(call include-path-for, audio-utils)

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog \
	libspeexresampler

LOCAL_CFLAGS := -Werror -Wall
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libaudioutils
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
	channels.c \
	fifo.c \
	format.c \
	limiter.c \
	minifloat.c \
	primitives.c \
	roundup.c
LOCAL_C_INCLUDES += \
	$(call include-path-for, audio-utils)
LOCAL_CFLAGS := -Werror -Wall
LOCAL_CFLAGS += -D__unused='__attribute__((unused))'
include $(BUILD_HOST_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libsndfile
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	tinysndfile.c

LOCAL_C_INCLUDES += \
	$(call include-path-for, audio-utils)

LOCAL_CFLAGS := -Werror -Wall
LOCAL_CFLAGS += -UHAVE_STDERR

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libsndfile
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	tinysndfile.c

LOCAL_C_INCLUDES += \
	$(call include-path-for, audio-utils)

#LOCAL_SHARED_LIBRARIES := libaudioutils

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_HOST_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libfifo
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	fifo.c \
	primitives.c \
	roundup.c

LOCAL_C_INCLUDES += \
	$(call include-path-for, audio-utils)

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_STATIC_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))

