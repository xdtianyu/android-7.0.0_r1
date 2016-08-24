LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libpdfiumopenjpeg

LOCAL_ARM_MODE := arm
LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays -fexceptions
LOCAL_CFLAGS += -Wno-non-virtual-dtor -Wall

# Mask some warnings. These are benign, but we probably want to fix them
# upstream at some point.
LOCAL_CFLAGS += -Wno-sign-compare -Wno-unused-parameter

LOCAL_SRC_FILES := \
    libopenjpeg20/bio.c \
    libopenjpeg20/cio.c \
    libopenjpeg20/dwt.c \
    libopenjpeg20/event.c \
    libopenjpeg20/function_list.c \
    libopenjpeg20/image.c \
    libopenjpeg20/invert.c \
    libopenjpeg20/j2k.c \
    libopenjpeg20/jp2.c \
    libopenjpeg20/mct.c \
    libopenjpeg20/mqc.c \
    libopenjpeg20/openjpeg.c \
    libopenjpeg20/opj_clock.c \
    libopenjpeg20/pi.c \
    libopenjpeg20/raw.c \
    libopenjpeg20/t1.c \
    libopenjpeg20/t2.c \
    libopenjpeg20/tcd.c \
    libopenjpeg20/tgt.c

LOCAL_C_INCLUDES := \
    external/pdfium

include $(BUILD_STATIC_LIBRARY)
