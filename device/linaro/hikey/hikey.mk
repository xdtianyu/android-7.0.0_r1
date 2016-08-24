ifeq ($(TARGET_KERNEL_USE_4_1), true)
TARGET_PREBUILT_KERNEL := device/linaro/hikey-kernel/Image-4.1
TARGET_PREBUILT_DTB := device/linaro/hikey-kernel/hi6220-hikey.dtb-4.1
TARGET_FSTAB := fstab.hikey-4.1
endif

#
# Inherit the full_base and device configurations
$(call inherit-product, device/linaro/hikey/device.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)

#
# Overrides
PRODUCT_NAME := hikey
PRODUCT_DEVICE := hikey
