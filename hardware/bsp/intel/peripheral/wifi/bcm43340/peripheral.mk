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
BCM43340_WIFI_FW_SRC := vendor/bsp/intel/peripheral/wifi/bcm43340_firmware
BCM43340_WIFI_FW_DST := system/vendor/firmware/bcm43340

# The "aob" file is special purpose - we include it here for completeness but don't utilize it.
#	$(BCM43340_WIFI_FW_SRC)/bcmdhd_aob.cal_4334x_b0:$(BCM43340_WIFI_FW_DST)/bcmdhd_aob.cal

PRODUCT_COPY_FILES += \
	$(BCM43340_WIFI_FW_SRC)/bcmdhd.cal_4334x_b0:$(BCM43340_WIFI_FW_DST)/bcmdhd.cal \
	$(BCM43340_WIFI_FW_SRC)/fw_bcmdhd_p2p.bin_4334x_b0:$(BCM43340_WIFI_FW_DST)/fw_bcmdhd_p2p.bin \
	$(BCM43340_WIFI_FW_SRC)/fw_bcmdhd_p2p.bin_4334x_b0:$(BCM43340_WIFI_FW_DST)/fw_bcmdhd_apsta.bin \
	$(BCM43340_WIFI_FW_SRC)/fw_bcmdhd_p2p.bin_4334x_b0:$(BCM43340_WIFI_FW_DST)/fw_bcmdhd_sta.bin

WIFI_DRIVER_HAL_MODULE := wifi_driver.edison
WIFI_DRIVER_HAL_PERIPHERAL := bcm43340
