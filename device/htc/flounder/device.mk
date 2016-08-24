#
# Copyright (C) 2013 The Android Open-Source Project
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

PRODUCT_PACKAGES := \
    libwpa_client \
    hostapd \
    wpa_supplicant \
    wpa_supplicant.conf

ifeq ($(TARGET_PREBUILT_KERNEL),)
ifeq ($(USE_SVELTE_KERNEL), true)
LOCAL_KERNEL := device/htc/flounder_svelte-kernel/Image.gz-dtb
else
LOCAL_KERNEL := device/htc/flounder-kernel/Image.gz-dtb
endif # USE_SVELTE_KERNEL
else
LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

BOARD_USERDATAIMAGE_FILE_SYSTEM_TYPE := f2fs

# This ensures the needed build tools are available.
# TODO: make non-linux builds happy with external/f2fs-tool; system/extras/f2fs_utils
ifeq ($(HOST_OS),linux)
TARGET_USERIMAGES_USE_F2FS := true
endif

LOCAL_FSTAB := $(LOCAL_PATH)/fstab.flounder


TARGET_RECOVERY_FSTAB = $(LOCAL_FSTAB)

PRODUCT_COPY_FILES := \
    $(LOCAL_KERNEL):kernel \
    $(LOCAL_PATH)/init.flounder.rc:root/init.flounder.rc \
    $(LOCAL_PATH)/init.flounder.usb.rc:root/init.flounder.usb.rc \
    $(LOCAL_PATH)/init.recovery.flounder.rc:root/init.recovery.flounder.rc \
    $(LOCAL_FSTAB):root/fstab.flounder \
    $(LOCAL_PATH)/ueventd.flounder.rc:root/ueventd.flounder.rc

# Copy flounder files as flounder64 so that ${ro.hardware} can find them
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/init.flounder.rc:root/init.flounder64.rc \
    $(LOCAL_PATH)/init.flounder.usb.rc:root/init.flounder64.usb.rc \
    $(LOCAL_FSTAB):root/fstab.flounder64 \
    $(LOCAL_PATH)/init.recovery.flounder.rc:root/init.recovery.flounder64.rc \
    $(LOCAL_PATH)/ueventd.flounder.rc:root/ueventd.flounder64.rc

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/touch/touch_fusion.cfg:$(TARGET_COPY_OUT_VENDOR)/firmware/touch_fusion.cfg \
    $(LOCAL_PATH)/touch/maxim_fp35.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/maxim_fp35.bin \
    $(LOCAL_PATH)/touch/touch_fusion.idc:system/usr/idc/touch_fusion.idc \
    $(LOCAL_PATH)/touch/touch_fusion:$(TARGET_COPY_OUT_VENDOR)/bin/touch_fusion \
    $(LOCAL_PATH)/touch/synaptics_dsx.idc:system/usr/idc/synaptics_dsx.idc

# headset keylayout
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/h2w_headset.kl:system/usr/keylayout/h2w_headset.kl

PRODUCT_PACKAGES += \
    libwpa_client \
    hostapd \
    wpa_supplicant \
    wpa_supplicant.conf

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.camera.full.xml:system/etc/permissions/android.hardware.camera.full.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.barometer.xml:system/etc/permissions/android.hardware.sensor.barometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.stepcounter.xml:system/etc/permissions/android.hardware.sensor.stepcounter.xml \
    frameworks/native/data/etc/android.hardware.sensor.stepdetector.xml:system/etc/permissions/android.hardware.sensor.stepdetector.xml \
    frameworks/native/data/etc/android.hardware.audio.low_latency.xml:system/etc/permissions/android.hardware.audio.low_latency.xml \
    frameworks/native/data/etc/android.hardware.audio.pro.xml:system/etc/permissions/android.hardware.audio.pro.xml \
    frameworks/native/data/etc/android.hardware.nfc.xml:system/etc/permissions/android.hardware.nfc.xml \
    frameworks/native/data/etc/android.hardware.nfc.hce.xml:system/etc/permissions/android.hardware.nfc.hce.xml \
    frameworks/native/data/etc/android.hardware.nfc.hcef.xml:system/etc/permissions/android.hardware.nfc.hcef.xml \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:system/etc/permissions/android.hardware.bluetooth_le.xml \
    frameworks/native/data/etc/android.hardware.opengles.aep.xml:system/etc/permissions/android.hardware.opengles.aep.xml \
    frameworks/native/data/etc/android.software.midi.xml:system/etc/permissions/android.software.midi.xml \
    $(LOCAL_PATH)/com.nvidia.nvsi.xml:system/etc/permissions/com.nvidia.nvsi.xml

PRODUCT_COPY_FILES += \
    frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml \
    $(LOCAL_PATH)/media_codecs.xml:system/etc/media_codecs.xml \
    $(LOCAL_PATH)/media_codecs_performance.xml:system/etc/media_codecs_performance.xml \
    $(LOCAL_PATH)/media_profiles.xml:system/etc/media_profiles.xml \
    $(LOCAL_PATH)/audio_policy.conf:system/etc/audio_policy.conf \
    $(LOCAL_PATH)/mixer_paths_0.xml:system/etc/mixer_paths_0.xml

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/enctune.conf:system/etc/enctune.conf

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/nvaudio_conf.xml:system/etc/nvaudio_conf.xml \
    $(LOCAL_PATH)/nvcamera.conf:system/etc/nvcamera.conf \
    $(LOCAL_PATH)/bcmdhd.cal:system/etc/wifi/bcmdhd.cal \
    $(LOCAL_PATH)/bcmdhd_lte.cal:system/etc/wifi/bcmdhd_lte.cal

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/bluetooth/bcm4350b1.hcd:$(TARGET_COPY_OUT_VENDOR)/firmware/bcm4350b1.hcd \
    $(LOCAL_PATH)/bluetooth/BCM4354_003.001.012.0319.0690_ORC.hcd:$(TARGET_COPY_OUT_VENDOR)/firmware/bcm4350c0.hcd \
    $(LOCAL_PATH)/bluetooth/bcm4354.hcd:$(TARGET_COPY_OUT_VENDOR)/firmware/bcm4354.hcd \
    $(LOCAL_PATH)/bluetooth/bt_vendor.conf:$(TARGET_COPY_OUT_SYSTEM)/etc/bluetooth/bt_vendor.conf

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/gps/bcm/gps.bcm47521.conf:system/etc/gps.bcm47521.conf \
    $(LOCAL_PATH)/gps/bcm/glgps:system/bin/glgps \
    $(LOCAL_PATH)/gps/bcm/gpsconfig.xml:system/etc/gpsconfig.xml \
    $(LOCAL_PATH)/gps/bcm/lib64/gps.bcm47521.so:system/lib64/hw/gps.bcm47521.so

# GPS: QCT MODULES
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/gps/qct/gps.conf:system/etc/gps.conf \
    $(LOCAL_PATH)/gps/qct/SuplRootCert:system/etc/SuplRootCert \
    $(LOCAL_PATH)/gps/qct/lib/libgeofence.so:system/lib/libgeofence.so \
    $(LOCAL_PATH)/gps/qct/lib/libgps.utils.so:system/lib/libgps.utils.so \
    $(LOCAL_PATH)/gps/qct/lib/libloc_api_v02.so:system/lib/libloc_api_v02.so \
    $(LOCAL_PATH)/gps/qct/lib/libloc_core.so:system/lib/libloc_core.so \
    $(LOCAL_PATH)/gps/qct/lib/libloc_ds_api.so:system/lib/libloc_ds_api.so \
    $(LOCAL_PATH)/gps/qct/lib/libloc_eng.so:system/lib/libloc_eng.so \
    $(LOCAL_PATH)/gps/qct/lib/hw/gps.default.so:system/lib/hw/gps.default.so \
    $(LOCAL_PATH)/gps/qct/lib64/libgeofence.so:system/lib64/libgeofence.so \
    $(LOCAL_PATH)/gps/qct/lib64/libgps.utils.so:system/lib64/libgps.utils.so \
    $(LOCAL_PATH)/gps/qct/lib64/libloc_api_v02.so:system/lib64/libloc_api_v02.so \
    $(LOCAL_PATH)/gps/qct/lib64/libloc_core.so:system/lib64/libloc_core.so \
    $(LOCAL_PATH)/gps/qct/lib64/libloc_ds_api.so:system/lib64/libloc_ds_api.so \
    $(LOCAL_PATH)/gps/qct/lib64/libloc_eng.so:system/lib64/libloc_eng.so \
    $(LOCAL_PATH)/gps/qct/lib64/hw/gps.default.so:system/lib64/hw/gps.default.so \
    $(LOCAL_PATH)/gps/qct/lib_vendor/libmdmdetect.so:vendor/lib/libmdmdetect.so \
    $(LOCAL_PATH)/gps/qct/lib_vendor/libperipheral_client.so:vendor/lib/libperipheral_client.so \
    $(LOCAL_PATH)/gps/qct/lib64_vendor/libmdmdetect.so:vendor/lib64/libmdmdetect.so \
    $(LOCAL_PATH)/gps/qct/lib64_vendor/libperipheral_client.so:vendor/lib64/libperipheral_client.so


# NFC feature + config files
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.nfc.xml:system/etc/permissions/android.hardware.nfc.xml \
    frameworks/native/data/etc/android.hardware.nfc.hce.xml:system/etc/permissions/android.hardware.nfc.hce.xml \
    device/htc/flounder/nfc/libnfc-brcm.conf:system/etc/libnfc-brcm.conf \
    device/htc/flounder/nfc/libnfc-brcm-20795a10.conf:system/etc/libnfc-brcm-20795a10.conf

PRODUCT_AAPT_CONFIG := normal large xlarge
PRODUCT_AAPT_PREF_CONFIG := xhdpi

PRODUCT_CHARACTERISTICS := tablet,nosdcard

ifneq ($(filter volantis volantisf, $(TARGET_PRODUCT)),)
# Wifi-Only overlays.
DEVICE_PACKAGE_OVERLAYS := \
    $(LOCAL_PATH)/wifi_only_overlay \
    $(LOCAL_PATH)/overlay
else
DEVICE_PACKAGE_OVERLAYS := \
    $(LOCAL_PATH)/overlay
endif

# NFC packages
PRODUCT_PACKAGES += \
    nfc_nci.bcm2079x.default \
    NfcNci \
    Tag \

PRODUCT_PACKAGES += \
    librs_jni \
    com.android.future.usb.accessory

PRODUCT_PACKAGES += \
    power.flounder \
    lights.flounder \
    sensors.flounder \
    thermal.flounder

# Filesystem management tools
PRODUCT_PACKAGES += \
    fsck.f2fs mkfs.f2fs

PRODUCT_PROPERTY_OVERRIDES := \
    wifi.interface=wlan0 \
    ro.opengles.version=196609 \
    ro.sf.lcd_density=320 \
    ro.hwui.texture_cache_size=72 \
    ro.hwui.layer_cache_size=48 \
    ro.hwui.r_buffer_cache_size=8 \
    ro.hwui.path_cache_size=32 \
    ro.hwui.gradient_cache_size=1 \
    ro.hwui.drop_shadow_cache_size=6 \
    ro.hwui.texture_cache_flushrate=0.4 \
    ro.hwui.text_small_cache_width=1024 \
    ro.hwui.text_small_cache_height=1024 \
    ro.hwui.text_large_cache_width=2048 \
    ro.hwui.text_large_cache_height=1024 \
    ro.hwui.disable_scissor_opt=true \
    ro.bt.bdaddr_path=/sys/module/flounder_bdaddress/parameters/bdaddress \
    ro.frp.pst=/dev/block/platform/sdhci-tegra.3/by-name/PST \
    ro.ril.def.agps.mode=1 \
    persist.tegra.compositor=glcomposer

# setup dalvik vm configs.
$(call inherit-product, frameworks/native/build/tablet-10in-xhdpi-2048-dalvik-heap.mk)

# for off charging mode
PRODUCT_PACKAGES += \
    charger \
    charger_res_images

# for audio
PRODUCT_PACKAGES += \
    audio.primary.flounder \
    audio.a2dp.default \
    audio.usb.default \
    audio.r_submix.default \
    libhtcacoustic \
    libnvvisualizer
PRODUCT_PROPERTY_OVERRIDES += \
    ro.audio.monitorRotation=true

# for sound trigger on DSP
PRODUCT_PACKAGES += \
    sound_trigger.primary.flounder

# for keyboard key mappings
PRODUCT_PACKAGES += \
	VolantisKeyboard

# for launcher layout
PRODUCT_PACKAGES += \
    VolantisLayout

# Allows healthd to boot directly from charger mode rather than initiating a reboot.
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    ro.enable_boot_charger_mode=1

# Reduce client buffer size for fast audio output tracks
# and configure audio low latency for 128 frames per buffer
PRODUCT_PROPERTY_OVERRIDES += \
    af.fast_track_multiplier=1 \
    audio_hal.period_size=128

# In userdebug, add minidebug info the the boot image and the system server to support
# diagnosing native crashes.
ifneq (,$(filter userdebug, $(TARGET_BUILD_VARIANT)))
    # Boot image.
    PRODUCT_DEX_PREOPT_BOOT_FLAGS += --generate-mini-debug-info
    # System server and some of its services.
    # Note: we cannot use PRODUCT_SYSTEM_SERVER_JARS, as it has not been expanded at this point.
    $(call add-product-dex-preopt-module-config,services,--generate-mini-debug-info)
    $(call add-product-dex-preopt-module-config,wifi-service,--generate-mini-debug-info)
endif

# add verity dependencies
$(call inherit-product, build/target/product/verity.mk)
PRODUCT_SUPPORTS_BOOT_SIGNER := false
PRODUCT_SYSTEM_VERITY_PARTITION := /dev/block/platform/sdhci-tegra.3/by-name/APP
PRODUCT_VENDOR_VERITY_PARTITION := /dev/block/platform/sdhci-tegra.3/by-name/VNR

# for warning
PRODUCT_PACKAGES += \
    slideshow \
    verity_warning_images

$(call inherit-product-if-exists, hardware/nvidia/tegra132/tegra132.mk)
$(call inherit-product-if-exists, vendor/nvidia/proprietary-tegra132/tegra132-vendor.mk)
$(call inherit-product-if-exists, vendor/htc/flounder/device-vendor.mk)
$(call inherit-product-if-exists, hardware/broadcom/wlan/bcmdhd/firmware/bcm4354/device-bcm.mk)
$(call inherit-product-if-exists, vendor/htc/flounder/audio/lifevibes/lvve/device-vendor-lvve.mk)
$(call inherit-product-if-exists, vendor/htc/flounder/audio/tfa/device-vendor-tfa.mk)

# Add dependency of the proprietary keystore.flounder module.
PRODUCT_PACKAGES += \
    libkeymaster_messages
