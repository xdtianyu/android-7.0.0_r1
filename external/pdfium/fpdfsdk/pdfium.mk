LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libpdfium

LOCAL_ARM_MODE := arm
LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays -fexceptions
LOCAL_CFLAGS += -Wno-non-virtual-dtor -Wall -DOPJ_STATIC \
                -DV8_DEPRECATION_WARNINGS -D_CRT_SECURE_NO_WARNINGS

# Mask some warnings. These are benign, but we probably want to fix them
# upstream at some point.
LOCAL_CFLAGS += -Wno-sign-compare -Wno-unused-parameter
LOCAL_CLANG_CFLAGS += -Wno-sign-compare

LOCAL_STATIC_LIBRARIES := libpdfiumformfiller \
                          libpdfiumpdfwindow \
                          libpdfiumjavascript \
                          libpdfiumfpdfapi \
                          libpdfiumfxge \
                          libpdfiumfxedit \
                          libpdfiumfpdftext \
                          libpdfiumfxcrt \
                          libpdfiumfxcodec \
                          libpdfiumfpdfdoc \
                          libpdfiumfdrm \
                          libpdfiumagg23 \
                          libpdfiumbigint \
                          libpdfiumlcms \
                          libpdfiumjpeg \
                          libpdfiumopenjpeg \
                          libpdfiumzlib


# TODO: figure out why turning on exceptions requires manually linking libdl
LOCAL_SHARED_LIBRARIES := libdl libft2

LOCAL_SRC_FILES := \
    src/fpdf_dataavail.cpp \
    src/fpdf_ext.cpp \
    src/fpdf_flatten.cpp \
    src/fpdf_progressive.cpp \
    src/fpdf_searchex.cpp \
    src/fpdf_sysfontinfo.cpp \
    src/fpdf_transformpage.cpp \
    src/fpdfdoc.cpp \
    src/fpdfeditimg.cpp \
    src/fpdfeditpage.cpp \
    src/fpdfformfill.cpp \
    src/fpdfppo.cpp \
    src/fpdfsave.cpp \
    src/fpdftext.cpp \
    src/fpdfview.cpp \
    src/fsdk_actionhandler.cpp \
    src/fsdk_annothandler.cpp \
    src/fsdk_baseannot.cpp \
    src/fsdk_baseform.cpp \
    src/fsdk_mgr.cpp \
    src/fsdk_rendercontext.cpp

LOCAL_C_INCLUDES := \
    external/pdfium \
    external/freetype/include \
    external/freetype/include/freetype

include $(BUILD_SHARED_LIBRARY)
