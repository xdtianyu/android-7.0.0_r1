#
# Copyright (C) 2011 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Adjust the dalvik heap to be appropriate for a tablet.
$(call inherit-product-if-exists, frameworks/native/build/tablet-10in-xhdpi-2048-dalvik-heap.mk)

ifeq ($(TARGET_PREBUILT_KERNEL),)
LOCAL_KERNEL := device/linaro/hikey-kernel/Image
LOCAL_DTB := device/linaro/hikey-kernel/hi6220-hikey.dtb
LOCAL_FSTAB := fstab.hikey
else
LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
LOCAL_DTB := $(TARGET_PREBUILT_DTB)
LOCAL_FSTAB := $(TARGET_FSTAB)
endif

PRODUCT_COPY_FILES +=   $(LOCAL_KERNEL):kernel \
                        $(LOCAL_DTB):hi6220-hikey.dtb \
			$(LOCAL_PATH)/$(LOCAL_FSTAB):root/fstab.hikey \
			$(LOCAL_PATH)/init.hikey.rc:root/init.hikey.rc \
			$(LOCAL_PATH)/init.hikey.usb.rc:root/init.hikey.usb.rc \
			$(LOCAL_PATH)/ueventd.hikey.rc:root/ueventd.hikey.rc \
			$(LOCAL_PATH)/hikey.kl:system/usr/keylayout/hikey.kl

# Set custom settings
DEVICE_PACKAGE_OVERLAYS := device/linaro/hikey/overlay

# Add openssh support for remote debugging and job submission
PRODUCT_PACKAGES += ssh sftp scp sshd ssh-keygen sshd_config start-ssh uim

# Add wifi-related packages
PRODUCT_PACKAGES += libwpa_client wpa_supplicant hostapd
PRODUCT_PROPERTY_OVERRIDES += wifi.interface=wlan0 \
                              wifi.supplicant_scan_interval=15

# Build and run only ART
PRODUCT_RUNTIMES := runtime_libart_default

# Build BT a2dp audio HAL
PRODUCT_PACKAGES += audio.a2dp.default

# Include USB speed switch App
PRODUCT_PACKAGES += UsbSpeedSwitch

# Build libion
PRODUCT_PACKAGES += libion

# Build gralloc for hikey
PRODUCT_PACKAGES += gralloc.hikey

# Build Audio Hal for hikey
PRODUCT_PACKAGES += audio.primary.hikey

# Set zygote config
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.zygote=zygote64_32
PRODUCT_COPY_FILES += system/core/rootdir/init.zygote64_32.rc:root/init.zygote64_32.rc

PRODUCT_PACKAGES += libGLES_android

PRODUCT_PACKAGES +=	TIInit_11.8.32.bts \
			wl18xx-fw-4.bin \
			wl18xx-conf.bin

# Copy hardware config file(s)
PRODUCT_COPY_FILES +=  \
        device/linaro/hikey/etc/permissions/android.hardware.screen.xml:system/etc/permissions/android.hardware.screen.xml \
        frameworks/native/data/etc/android.hardware.ethernet.xml:system/etc/permissions/android.hardware.ethernet.xml \
        frameworks/native/data/etc/android.software.app_widgets.xml:system/etc/permissions/android.software.app_widgets.xml \
        frameworks/native/data/etc/android.software.backup.xml:system/etc/permissions/android.software.backup.xml \
        frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml

# Include vendor binaries
$(call inherit-product-if-exists, vendor/linaro/hikey/device-vendor.mk)

# Include BT modules
$(call inherit-product-if-exists, device/linaro/hikey/wpan/ti-wpan-products.mk)

PRODUCT_COPY_FILES += \
        frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
        frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
        frameworks/native/data/etc/android.hardware.bluetooth_le.xml:system/etc/permissions/android.hardware.bluetooth_le.xml \
        device/linaro/hikey/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
        device/linaro/hikey/audio/audio_policy.conf:system/etc/audio_policy.conf

# Copy media codecs config file
PRODUCT_COPY_FILES += device/linaro/hikey/etc/media_codecs.xml:system/etc/media_codecs.xml
