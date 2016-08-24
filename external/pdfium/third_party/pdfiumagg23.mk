LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libpdfiumagg23

LOCAL_ARM_MODE := arm
LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays -fexceptions
LOCAL_CFLAGS += -Wno-non-virtual-dtor -Wall

# Mask some warnings. These are benign, but we probably want to fix them
# upstream at some point.
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-unused-function

LOCAL_SRC_FILES := \
    agg23/agg_curves.cpp \
    agg23/agg_path_storage.cpp \
    agg23/agg_rasterizer_scanline_aa.cpp \
    agg23/agg_vcgen_dash.cpp \
    agg23/agg_vcgen_stroke.cpp \

LOCAL_C_INCLUDES := \
    external/pdfium

include $(BUILD_STATIC_LIBRARY)
