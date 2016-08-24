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

# Select the SoC
$(call set_soc, intel, edison)

# Add wifi controller
$(call add_peripheral, intel, wifi/bcm43340)
# Add bt controller
$(call add_peripheral, intel, bluetooth/bcm43340)
# Add lights HAL
$(call add_peripheral, intel, light/mraa)
# Add audio support
$(call add_peripheral, intel, audio/generic/usb)
# Add sensor support
$(call add_peripheral, intel, sensors/mraa)

BOARD_SYSTEMIMAGE_PARTITION_SIZE := 268435456
BOARD_USERDATAIMAGE_PARTITION_SIZE := 134217728
BOARD_CACHEIMAGE_PARTITION_SIZE := 268435456
BOARD_U_BOOT_ENV_SIZE := 0x10000
BOARD_GPT_INI := device/intel/edison/gpt.ini

# Specific sensors to be built into the sensors HAL
PLATFORM_SENSOR_LIST += GroveLight
PLATFORM_SENSOR_LIST += GroveTemperature
PLATFORM_SENSOR_LIST += MMA7660Accelerometer
PLATFORM_SENSOR_LIST += MPU9150Accelerometer
PLATFORM_SENSOR_LIST += LSM303dAccelerometer
PLATFORM_SENSOR_LIST += LSM303dOrientation
PLATFORM_SENSOR_LIST += ProximityGPIO

PRODUCT_COPY_FILES += \
    device/intel/edison/flash_tools/brillo-flashall-edison.sh:provision-device \
    device/intel/edison/fstab.device:root/fstab.${soc_name}

WIFI_DRIVER_NVRAM_PATH_PARAM := "/sys/module/bcm4334x/parameters/nvram_path"
WIFI_DRIVER_FW_PATH_PARAM    := "/sys/module/bcm4334x/parameters/firmware_path"

WIFI_DRIVER_NVRAM_PATH       := "/vendor/firmware/bcm43340/bcmdhd.cal"
WIFI_DRIVER_FW_PATH_STA      := "/vendor/firmware/bcm43340/fw_bcmdhd_sta.bin"
WIFI_DRIVER_FW_PATH_AP       := "/vendor/firmware/bcm43340/fw_bcmdhd_apsta.bin"
WIFI_DRIVER_FW_PATH_P2P      := "/vendor/firmware/bcm43340/fw_bcmdhd_p2p.bin"

BOARD_SEPOLICY_DIRS += \
    device/intel/edison/sepolicy

vendor_partition_directory := vendor/bsp/intel/edison
BRILLO_VENDOR_PARTITIONS := \
    $(vendor_partition_directory)/uboot_firmware:u-boot-edison.bin \
    $(vendor_partition_directory)/uboot_firmware:gpt.bin

# Must defined at the end of the file
$(call add_device_packages)
