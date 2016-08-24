#
# Copyright 2015 The Android Open-Source Project
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

$(call inherit-product, device/moto/shamu/aosp_shamu.mk)

# Add overlay to enable extra Bluetooth profiles
DEVICE_PACKAGE_OVERLAYS += \
    device/moto/shamu/overlay_bt

PRODUCT_NAME := bt_shamu
PRODUCT_DEVICE := shamu
PRODUCT_BRAND := Android
PRODUCT_MODEL := BT Shamu
PRODUCT_MANUFACTURER := motorola
PRODUCT_RESTRICT_VENDOR_FILES := true

AUDIO_FEATURE_ENABLED_HFP := true
