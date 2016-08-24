LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= libpdfiumfpdfapi

LOCAL_ARM_MODE := arm
LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays -fexceptions
LOCAL_CFLAGS += -Wno-non-virtual-dtor -Wall -DOPJ_STATIC \
                -DV8_DEPRECATION_WARNINGS -D_CRT_SECURE_NO_WARNINGS

# Mask some warnings. These are benign, but we probably want to fix them
# upstream at some point.
LOCAL_CFLAGS += -Wno-sign-compare -Wno-unused-parameter -Wno-missing-field-initializers
LOCAL_CLANG_CFLAGS += -Wno-sign-compare

LOCAL_SRC_FILES := \
    src/fpdfapi/fpdf_basic_module.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/Adobe-CNS1-UCS2_5.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/B5pc-H_0.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/B5pc-V_0.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/CNS-EUC-H_0.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/CNS-EUC-V_0.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/ETen-B5-H_0.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/ETen-B5-V_0.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/ETenms-B5-H_0.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/ETenms-B5-V_0.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/HKscs-B5-H_5.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/HKscs-B5-V_5.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/UniCNS-UCS2-H_3.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/UniCNS-UCS2-V_3.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/UniCNS-UTF16-H_0.cpp \
    src/fpdfapi/fpdf_cmaps/CNS1/cmaps_cns1.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/Adobe-GB1-UCS2_5.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/GB-EUC-H_0.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/GB-EUC-V_0.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/GBK-EUC-H_2.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/GBK-EUC-V_2.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/GBK2K-H_5.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/GBK2K-V_5.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/GBKp-EUC-H_2.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/GBKp-EUC-V_2.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/GBpc-EUC-H_0.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/GBpc-EUC-V_0.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/UniGB-UCS2-H_4.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/UniGB-UCS2-V_4.cpp \
    src/fpdfapi/fpdf_cmaps/GB1/cmaps_gb1.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/83pv-RKSJ-H_1.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/90ms-RKSJ-H_2.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/90ms-RKSJ-V_2.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/90msp-RKSJ-H_2.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/90msp-RKSJ-V_2.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/90pv-RKSJ-H_1.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/Add-RKSJ-H_1.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/Add-RKSJ-V_1.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/Adobe-Japan1-UCS2_4.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/EUC-H_1.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/EUC-V_1.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/Ext-RKSJ-H_2.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/Ext-RKSJ-V_2.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/H_1.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/UniJIS-UCS2-HW-H_4.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/UniJIS-UCS2-HW-V_4.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/UniJIS-UCS2-H_4.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/UniJIS-UCS2-V_4.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/V_1.cpp \
    src/fpdfapi/fpdf_cmaps/Japan1/cmaps_japan1.cpp \
    src/fpdfapi/fpdf_cmaps/Korea1/Adobe-Korea1-UCS2_2.cpp \
    src/fpdfapi/fpdf_cmaps/Korea1/KSC-EUC-H_0.cpp \
    src/fpdfapi/fpdf_cmaps/Korea1/KSC-EUC-V_0.cpp \
    src/fpdfapi/fpdf_cmaps/Korea1/KSCms-UHC-HW-H_1.cpp \
    src/fpdfapi/fpdf_cmaps/Korea1/KSCms-UHC-HW-V_1.cpp \
    src/fpdfapi/fpdf_cmaps/Korea1/KSCms-UHC-H_1.cpp \
    src/fpdfapi/fpdf_cmaps/Korea1/KSCms-UHC-V_1.cpp \
    src/fpdfapi/fpdf_cmaps/Korea1/KSCpc-EUC-H_0.cpp \
    src/fpdfapi/fpdf_cmaps/Korea1/UniKS-UCS2-H_1.cpp \
    src/fpdfapi/fpdf_cmaps/Korea1/UniKS-UCS2-V_1.cpp \
    src/fpdfapi/fpdf_cmaps/Korea1/UniKS-UTF16-H_0.cpp \
    src/fpdfapi/fpdf_cmaps/Korea1/cmaps_korea1.cpp \
    src/fpdfapi/fpdf_cmaps/fpdf_cmaps.cpp \
    src/fpdfapi/fpdf_edit/fpdf_edit_content.cpp \
    src/fpdfapi/fpdf_edit/fpdf_edit_create.cpp \
    src/fpdfapi/fpdf_edit/fpdf_edit_doc.cpp \
    src/fpdfapi/fpdf_edit/fpdf_edit_image.cpp \
    src/fpdfapi/fpdf_font/fpdf_font.cpp \
    src/fpdfapi/fpdf_font/fpdf_font_charset.cpp \
    src/fpdfapi/fpdf_font/fpdf_font_cid.cpp \
    src/fpdfapi/fpdf_font/ttgsubtable.cpp \
    src/fpdfapi/fpdf_page/fpdf_page.cpp \
    src/fpdfapi/fpdf_page/fpdf_page_colors.cpp \
    src/fpdfapi/fpdf_page/fpdf_page_doc.cpp \
    src/fpdfapi/fpdf_page/fpdf_page_func.cpp \
    src/fpdfapi/fpdf_page/fpdf_page_graph_state.cpp \
    src/fpdfapi/fpdf_page/fpdf_page_image.cpp \
    src/fpdfapi/fpdf_page/fpdf_page_parser.cpp \
    src/fpdfapi/fpdf_page/fpdf_page_parser_old.cpp \
    src/fpdfapi/fpdf_page/fpdf_page_path.cpp \
    src/fpdfapi/fpdf_page/fpdf_page_pattern.cpp \
    src/fpdfapi/fpdf_parser/fpdf_parser_decode.cpp \
    src/fpdfapi/fpdf_parser/fpdf_parser_document.cpp \
    src/fpdfapi/fpdf_parser/fpdf_parser_encrypt.cpp \
    src/fpdfapi/fpdf_parser/fpdf_parser_fdf.cpp \
    src/fpdfapi/fpdf_parser/fpdf_parser_objects.cpp \
    src/fpdfapi/fpdf_parser/fpdf_parser_parser.cpp \
    src/fpdfapi/fpdf_parser/fpdf_parser_utility.cpp \
    src/fpdfapi/fpdf_render/fpdf_render.cpp \
    src/fpdfapi/fpdf_render/fpdf_render_cache.cpp \
    src/fpdfapi/fpdf_render/fpdf_render_image.cpp \
    src/fpdfapi/fpdf_render/fpdf_render_loadimage.cpp \
    src/fpdfapi/fpdf_render/fpdf_render_pattern.cpp \
    src/fpdfapi/fpdf_render/fpdf_render_text.cpp

LOCAL_C_INCLUDES := \
    external/pdfium \
    external/freetype/include \
    external/freetype/include/freetype

include $(BUILD_STATIC_LIBRARY)
