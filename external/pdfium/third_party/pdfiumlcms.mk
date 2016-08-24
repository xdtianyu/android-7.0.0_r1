LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libpdfiumlcms

LOCAL_ARM_MODE := arm
LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays -fexceptions
LOCAL_CFLAGS += -Wno-non-virtual-dtor -Wall

# Mask some warnings. These are benign, but we probably want to fix them
# upstream at some point.
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-missing-braces -Wno-unused-function

LOCAL_SRC_FILES := \
    lcms2-2.6/src/cmscam02.c \
    lcms2-2.6/src/cmscgats.c \
    lcms2-2.6/src/cmscnvrt.c \
    lcms2-2.6/src/cmserr.c \
    lcms2-2.6/src/cmsgamma.c \
    lcms2-2.6/src/cmsgmt.c \
    lcms2-2.6/src/cmshalf.c \
    lcms2-2.6/src/cmsintrp.c \
    lcms2-2.6/src/cmsio0.c \
    lcms2-2.6/src/cmsio1.c \
    lcms2-2.6/src/cmslut.c \
    lcms2-2.6/src/cmsmd5.c \
    lcms2-2.6/src/cmsmtrx.c \
    lcms2-2.6/src/cmsnamed.c \
    lcms2-2.6/src/cmsopt.c \
    lcms2-2.6/src/cmspack.c \
    lcms2-2.6/src/cmspcs.c \
    lcms2-2.6/src/cmsplugin.c \
    lcms2-2.6/src/cmsps2.c \
    lcms2-2.6/src/cmssamp.c \
    lcms2-2.6/src/cmssm.c \
    lcms2-2.6/src/cmstypes.c \
    lcms2-2.6/src/cmsvirt.c \
    lcms2-2.6/src/cmswtpnt.c \
    lcms2-2.6/src/cmsxform.c

LOCAL_C_INCLUDES := \
    external/pdfium

include $(BUILD_STATIC_LIBRARY)
