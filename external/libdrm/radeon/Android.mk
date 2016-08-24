LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Import variables LIBDRM_RADEON_FILES, LIBDRM_RADEON_H_FILES
include $(LOCAL_PATH)/Makefile.sources

LOCAL_MODULE := libdrm_radeon

LOCAL_SHARED_LIBRARIES := libdrm

LOCAL_SRC_FILES := $(filter-out %.h,$(LIBDRM_RADEON_FILES))

LOCAL_CFLAGS := \
	-DHAVE_LIBDRM_ATOMIC_PRIMITIVES=1

include $(BUILD_SHARED_LIBRARY)
