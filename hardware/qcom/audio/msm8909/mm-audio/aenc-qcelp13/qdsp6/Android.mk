ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#                 Common definitons
# ---------------------------------------------------------------------------------

libOmxQcelp13Enc-def := -g -O3
libOmxQcelp13Enc-def += -DQC_MODIFIED
libOmxQcelp13Enc-def += -D_ANDROID_
libOmxQcelp13Enc-def += -D_ENABLE_QC_MSG_LOG_
libOmxQcelp13Enc-def += -DVERBOSE
libOmxQcelp13Enc-def += -D_DEBUG
libOmxQcelp13Enc-def += -Wconversion
libOmxQcelp13Enc-def += -DAUDIOV2

# ---------------------------------------------------------------------------------
#             Make the Shared library (libOmxQcelp13Enc)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

libOmxQcelp13Enc-inc       := $(LOCAL_PATH)/inc
libOmxQcelp13Enc-inc       += $(TARGET_OUT_HEADERS)/mm-core/omxcore

LOCAL_MODULE            := libOmxQcelp13Enc
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxQcelp13Enc-def)
LOCAL_C_INCLUDES        := $(libOmxQcelp13Enc-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog

LOCAL_SRC_FILES         := src/aenc_svr.c
LOCAL_SRC_FILES         += src/omx_qcelp13_aenc.cpp

LOCAL_C_INCLUDES += $(BOARD_KERNEL_HEADER_DIR)
LOCAL_ADDITIONAL_DEPENDENCIES += $(BOARD_KERNEL_HEADER_DEPENDENCIES)

include $(BUILD_SHARED_LIBRARY)


# ---------------------------------------------------------------------------------
#             Make the apps-test (mm-aenc-omxqcelp13-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-qcelp13-enc-test-inc    := $(LOCAL_PATH)/inc
mm-qcelp13-enc-test-inc    += $(LOCAL_PATH)/test

mm-qcelp13-enc-test-inc    += $(TARGET_OUT_HEADERS)/mm-core/omxcore
mm-qcelp13-enc-test-inc    += $(TARGET_OUT_HEADERS)/mm-audio/audio-alsa
LOCAL_MODULE            := mm-aenc-omxqcelp13-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxQcelp13Enc-def)
LOCAL_C_INCLUDES        := $(mm-qcelp13-enc-test-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libmm-omxcore
LOCAL_SHARED_LIBRARIES  += libOmxQcelp13Enc
LOCAL_SHARED_LIBRARIES  += libaudioalsa
LOCAL_SRC_FILES         := test/omx_qcelp13_enc_test.c

include $(BUILD_EXECUTABLE)

endif

# ---------------------------------------------------------------------------------
#                     END
# ---------------------------------------------------------------------------------

