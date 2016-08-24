LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libpdfiumjpeg

LOCAL_ARM_MODE := arm
LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays -fexceptions
LOCAL_CFLAGS += -Wno-non-virtual-dtor -Wall

# Mask some warnings. These are benign, but we probably want to fix them
# upstream at some point.
LOCAL_CFLAGS += -Wno-shift-negative-value -Wno-unused-parameter

LOCAL_SRC_FILES := \
    libjpeg/fpdfapi_jcapimin.c \
    libjpeg/fpdfapi_jcapistd.c \
    libjpeg/fpdfapi_jccoefct.c \
    libjpeg/fpdfapi_jccolor.c \
    libjpeg/fpdfapi_jcdctmgr.c \
    libjpeg/fpdfapi_jchuff.c \
    libjpeg/fpdfapi_jcinit.c \
    libjpeg/fpdfapi_jcmainct.c \
    libjpeg/fpdfapi_jcmarker.c \
    libjpeg/fpdfapi_jcmaster.c \
    libjpeg/fpdfapi_jcomapi.c \
    libjpeg/fpdfapi_jcparam.c \
    libjpeg/fpdfapi_jcphuff.c \
    libjpeg/fpdfapi_jcprepct.c \
    libjpeg/fpdfapi_jcsample.c \
    libjpeg/fpdfapi_jctrans.c \
    libjpeg/fpdfapi_jdapimin.c \
    libjpeg/fpdfapi_jdapistd.c \
    libjpeg/fpdfapi_jdcoefct.c \
    libjpeg/fpdfapi_jdcolor.c \
    libjpeg/fpdfapi_jddctmgr.c \
    libjpeg/fpdfapi_jdhuff.c \
    libjpeg/fpdfapi_jdinput.c \
    libjpeg/fpdfapi_jdmainct.c \
    libjpeg/fpdfapi_jdmarker.c \
    libjpeg/fpdfapi_jdmaster.c \
    libjpeg/fpdfapi_jdmerge.c \
    libjpeg/fpdfapi_jdphuff.c \
    libjpeg/fpdfapi_jdpostct.c \
    libjpeg/fpdfapi_jdsample.c \
    libjpeg/fpdfapi_jdtrans.c \
    libjpeg/fpdfapi_jerror.c \
    libjpeg/fpdfapi_jfdctfst.c \
    libjpeg/fpdfapi_jfdctint.c \
    libjpeg/fpdfapi_jidctfst.c \
    libjpeg/fpdfapi_jidctint.c \
    libjpeg/fpdfapi_jidctred.c \
    libjpeg/fpdfapi_jmemmgr.c \
    libjpeg/fpdfapi_jmemnobs.c \
    libjpeg/fpdfapi_jutils.c

LOCAL_C_INCLUDES := \
    external/pdfium

include $(BUILD_STATIC_LIBRARY)
