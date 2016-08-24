#
# Copyright 2015 The Android Open Source Project
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

# BCM43340 WIFI Firmware
BCM43340_BT_SRC := hardware/bsp/intel/peripheral/bluetooth/bcm43340
BCM43340_BT_FIRMWARE := vendor/bsp/intel/peripheral/bluetooth/bcm43340_firmware
BCM43340_BT_FW_DST := system/vendor/firmware/bcm43340

PRODUCT_COPY_FILES += \
    $(BCM43340_BT_FIRMWARE)/BCM43341B0_002.001.014.0122.0166.hcd:$(BCM43340_BT_FW_DST)/BCM43341B0_002.001.014.0122.0166.hcd \
    $(BCM43340_BT_SRC)/bt_vendor.conf:system/etc/bluetooth/bt_vendor.conf

#BCM bluetooth
BOARD_HAVE_BLUETOOTH_BCM := true
BOARD_CUSTOM_BT_CONFIG := $(BCM43340_BT_SRC)/vnd_edison.txt
