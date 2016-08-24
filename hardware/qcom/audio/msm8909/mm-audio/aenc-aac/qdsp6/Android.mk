ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#                 Common definitons
# ---------------------------------------------------------------------------------

libOmxAacEnc-def := -g -O3
libOmxAacEnc-def += -DQC_MODIFIED
libOmxAacEnc-def += -D_ANDROID_
libOmxAacEnc-def += -D_ENABLE_QC_MSG_LOG_
libOmxAacEnc-def += -DVERBOSE
libOmxAacEnc-def += -D_DEBUG
libOmxAacEnc-def += -Wconversion
libOmxAacEnc-def += -DAUDIOV2

# ---------------------------------------------------------------------------------
#             Make the Shared library (libOmxAacEnc)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

libOmxAacEnc-inc       := $(LOCAL_PATH)/inc
libOmxAacEnc-inc       += $(TARGET_OUT_HEADERS)/mm-core/omxcore

LOCAL_MODULE            := libOmxAacEnc
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxAacEnc-def)
LOCAL_C_INCLUDES        := $(libOmxAacEnc-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog

LOCAL_SRC_FILES         := src/aenc_svr.c
LOCAL_SRC_FILES         += src/omx_aac_aenc.cpp

LOCAL_C_INCLUDES += $(BOARD_KERNEL_HEADER_DIR)
LOCAL_ADDITIONAL_DEPENDENCIES += $(BOARD_KERNEL_HEADER_DEPENDENCIES)

include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
#             Make the apps-test (mm-aenc-omxaac-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-aac-enc-test-inc    := $(LOCAL_PATH)/inc
mm-aac-enc-test-inc    += $(LOCAL_PATH)/test
mm-aac-enc-test-inc    += $(TARGET_OUT_HEADERS)/mm-audio/audio-alsa
mm-aac-enc-test-inc    += $(TARGET_OUT_HEADERS)/mm-core/omxcore

LOCAL_MODULE            := mm-aenc-omxaac-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxAacEnc-def)
LOCAL_C_INCLUDES        := $(mm-aac-enc-test-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libmm-omxcore
LOCAL_SHARED_LIBRARIES  += libOmxAacEnc
LOCAL_SHARED_LIBRARIES  += libaudioalsa
LOCAL_SRC_FILES         := test/omx_aac_enc_test.c

include $(BUILD_EXECUTABLE)

endif

# ---------------------------------------------------------------------------------
#                     END
# ---------------------------------------------------------------------------------

