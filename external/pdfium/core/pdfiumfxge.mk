LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= libpdfiumfxge

LOCAL_ARM_MODE := arm
LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays -fexceptions
LOCAL_CFLAGS += -Wno-non-virtual-dtor -Wall -DOPJ_STATIC \
                -DV8_DEPRECATION_WARNINGS -D_CRT_SECURE_NO_WARNINGS

# Mask some warnings. These are benign, but we probably want to fix them
# upstream at some point.
LOCAL_CFLAGS += -Wno-sign-compare -Wno-unused-parameter
LOCAL_CLANG_CFLAGS += -Wno-sign-compare -Wno-switch

LOCAL_SRC_FILES := \
    src/fxge/agg/src/fx_agg_driver.cpp \
    src/fxge/android/fpf_skiafont.cpp \
    src/fxge/android/fpf_skiafontmgr.cpp \
    src/fxge/android/fpf_skiamodule.cpp \
    src/fxge/android/fx_android_font.cpp \
    src/fxge/android/fx_android_imp.cpp \
    src/fxge/apple/fx_apple_platform.cpp \
    src/fxge/apple/fx_mac_imp.cpp \
    src/fxge/apple/fx_quartz_device.cpp \
    src/fxge/dib/fx_dib_composite.cpp \
    src/fxge/dib/fx_dib_convert.cpp \
    src/fxge/dib/fx_dib_engine.cpp \
    src/fxge/dib/fx_dib_main.cpp \
    src/fxge/dib/fx_dib_transform.cpp \
    src/fxge/fontdata/chromefontdata/FoxitDingbats.c \
    src/fxge/fontdata/chromefontdata/FoxitFixed.c \
    src/fxge/fontdata/chromefontdata/FoxitFixedBold.c \
    src/fxge/fontdata/chromefontdata/FoxitFixedBoldItalic.c \
    src/fxge/fontdata/chromefontdata/FoxitFixedItalic.c \
    src/fxge/fontdata/chromefontdata/FoxitSans.c \
    src/fxge/fontdata/chromefontdata/FoxitSansBold.c \
    src/fxge/fontdata/chromefontdata/FoxitSansBoldItalic.c \
    src/fxge/fontdata/chromefontdata/FoxitSansItalic.c \
    src/fxge/fontdata/chromefontdata/FoxitSansMM.c \
    src/fxge/fontdata/chromefontdata/FoxitSerif.c \
    src/fxge/fontdata/chromefontdata/FoxitSerifBold.c \
    src/fxge/fontdata/chromefontdata/FoxitSerifBoldItalic.c \
    src/fxge/fontdata/chromefontdata/FoxitSerifItalic.c \
    src/fxge/fontdata/chromefontdata/FoxitSerifMM.c \
    src/fxge/fontdata/chromefontdata/FoxitSymbol.c \
    src/fxge/freetype/fx_freetype.c \
    src/fxge/ge/fx_ge.cpp \
    src/fxge/ge/fx_ge_device.cpp \
    src/fxge/ge/fx_ge_font.cpp \
    src/fxge/ge/fx_ge_fontmap.cpp \
    src/fxge/ge/fx_ge_linux.cpp \
    src/fxge/ge/fx_ge_path.cpp \
    src/fxge/ge/fx_ge_ps.cpp \
    src/fxge/ge/fx_ge_text.cpp

LOCAL_C_INCLUDES := \
    external/pdfium \
    external/freetype/include \
    external/freetype/include/freetype

include $(BUILD_STATIC_LIBRARY)
