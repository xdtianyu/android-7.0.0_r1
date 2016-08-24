LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= libpdfiumfxcrt

LOCAL_ARM_MODE := arm
LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays -fexceptions
LOCAL_CFLAGS += -Wno-non-virtual-dtor -Wall -DOPJ_STATIC \
                -DV8_DEPRECATION_WARNINGS -D_CRT_SECURE_NO_WARNINGS
LOCAL_CFLAGS_arm64 += -D_FX_CPU_=_FX_X64_ -fPIC

# Mask some warnings. These are benign, but we probably want to fix them
# upstream at some point.
LOCAL_CFLAGS += -Wno-sign-compare -Wno-unused-parameter
LOCAL_CLANG_CFLAGS += -Wno-sign-compare

LOCAL_SRC_FILES := \
    src/fxcrt/fx_basic_array.cpp \
    src/fxcrt/fx_basic_bstring.cpp \
    src/fxcrt/fx_basic_buffer.cpp \
    src/fxcrt/fx_basic_coords.cpp \
    src/fxcrt/fx_basic_gcc.cpp \
    src/fxcrt/fx_basic_list.cpp \
    src/fxcrt/fx_basic_maps.cpp \
    src/fxcrt/fx_basic_memmgr.cpp \
    src/fxcrt/fx_basic_plex.cpp \
    src/fxcrt/fx_basic_utf.cpp \
    src/fxcrt/fx_basic_util.cpp \
    src/fxcrt/fx_basic_wstring.cpp \
    src/fxcrt/fx_bidi.cpp \
    src/fxcrt/fx_extension.cpp \
    src/fxcrt/fx_ucddata.cpp \
    src/fxcrt/fx_unicode.cpp \
    src/fxcrt/fx_xml_composer.cpp \
    src/fxcrt/fx_xml_parser.cpp \
    src/fxcrt/fxcrt_platforms.cpp \
    src/fxcrt/fxcrt_posix.cpp \
    src/fxcrt/fxcrt_windows.cpp

LOCAL_C_INCLUDES := \
    external/pdfium \
    external/freetype/include \
    external/freetype/include/freetype

include $(BUILD_STATIC_LIBRARY)
