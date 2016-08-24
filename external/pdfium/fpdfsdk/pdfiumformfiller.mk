LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= libpdfiumformfiller

LOCAL_ARM_MODE := arm
LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays -fexceptions
LOCAL_CFLAGS += -Wno-non-virtual-dtor -Wall -DOPJ_STATIC \
                -DV8_DEPRECATION_WARNINGS -D_CRT_SECURE_NO_WARNINGS

# Mask some warnings. These are benign, but we probably want to fix them
# upstream at some point.
LOCAL_CFLAGS += -Wno-sign-compare -Wno-unused-parameter
LOCAL_CLANG_CFLAGS += -Wno-sign-compare

LOCAL_SRC_FILES := \
    src/formfiller/FFL_CBA_Fontmap.cpp \
    src/formfiller/FFL_CheckBox.cpp \
    src/formfiller/FFL_ComboBox.cpp \
    src/formfiller/FFL_FormFiller.cpp \
    src/formfiller/FFL_IFormFiller.cpp \
    src/formfiller/FFL_ListBox.cpp \
    src/formfiller/FFL_PushButton.cpp \
    src/formfiller/FFL_RadioButton.cpp \
    src/formfiller/FFL_TextField.cpp

LOCAL_C_INCLUDES := \
    external/pdfium \
    external/freetype/include \
    external/freetype/include/freetype

include $(BUILD_STATIC_LIBRARY)
