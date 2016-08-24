LOCAL_PATH:= $(call my-dir)
NFA := src/nfa
NFC := src/nfc
HAL := src/hal
UDRV := src/udrv
HALIMPL := halimpl/bcm2079x
D_CFLAGS := -DANDROID -DBUILDCFG=1 \
    -Wno-deprecated-register \
    -Wno-unused-parameter \


######################################
# Build shared library system/lib/libnfc-nci.so for stack code.

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm
LOCAL_MODULE := libnfc-nci
LOCAL_SHARED_LIBRARIES := libhardware_legacy libcutils liblog libdl libhardware
LOCAL_CFLAGS := $(D_CFLAGS)
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/src/include \
    $(LOCAL_PATH)/src/gki/ulinux \
    $(LOCAL_PATH)/src/gki/common \
    $(LOCAL_PATH)/$(NFA)/include \
    $(LOCAL_PATH)/$(NFA)/int \
    $(LOCAL_PATH)/$(NFC)/include \
    $(LOCAL_PATH)/$(NFC)/int \
    $(LOCAL_PATH)/src/hal/include \
    $(LOCAL_PATH)/src/hal/int \
    $(LOCAL_PATH)/$(HALIMPL)/include
LOCAL_SRC_FILES := \
    $(call all-c-files-under, $(NFA)/ce $(NFA)/dm $(NFA)/ee) \
    $(call all-c-files-under, $(NFA)/hci $(NFA)/int $(NFA)/p2p $(NFA)/rw $(NFA)/sys) \
    $(call all-c-files-under, $(NFC)/int $(NFC)/llcp $(NFC)/nci $(NFC)/ndef $(NFC)/nfc $(NFC)/tags) \
    $(call all-c-files-under, src/adaptation) \
    $(call all-cpp-files-under, src/adaptation) \
    $(call all-c-files-under, src/gki) \
    $(HALIMPL)/adaptation/android_logmsg.cpp \
    src/nfca_version.c
include $(BUILD_SHARED_LIBRARY)


######################################
# Build shared library system/lib/hw/nfc_nci.*.so for Hardware Abstraction Layer.
# Android's generic HAL (libhardware.so) dynamically loads this shared library.

include $(CLEAR_VARS)
LOCAL_MODULE := nfc_nci.bcm2079x.default
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := $(call all-c-files-under, $(HALIMPL)) \
    $(call all-cpp-files-under, $(HALIMPL)) \
    src/adaptation/CrcChecksum.cpp \
    src//nfca_version.c
LOCAL_SHARED_LIBRARIES := liblog libcutils libhardware_legacy
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/$(HALIMPL)/include \
    $(LOCAL_PATH)/$(HALIMPL)/gki/ulinux \
    $(LOCAL_PATH)/$(HALIMPL)/gki/common \
    $(LOCAL_PATH)/$(HAL)/include \
    $(LOCAL_PATH)/$(HAL)/int \
    $(LOCAL_PATH)/src/include \
    $(LOCAL_PATH)/$(NFC)/include \
    $(LOCAL_PATH)/$(NFA)/include \
    $(LOCAL_PATH)/$(UDRV)/include
LOCAL_CFLAGS := $(D_CFLAGS) -DNFC_HAL_TARGET=TRUE -DNFC_RW_ONLY=TRUE
include $(BUILD_SHARED_LIBRARY)


######################################
include $(call all-makefiles-under,$(LOCAL_PATH))
