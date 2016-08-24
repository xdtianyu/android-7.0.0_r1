# Copyright (C) 2015 Intel Corporation
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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPPFLAGS:= -Wno-unused-parameter -Wno-error=non-virtual-dtor -fexceptions
LOCAL_CFLAGS += -DLOG_TAG=\"Sensors\" -Wno-unused-parameter
LOCAL_SHARED_LIBRARIES := libcutils libupm libmraa
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../libupm/src/mpu9150/
LOCAL_SRC_FILES := SensorsHAL.cpp Sensor.cpp AcquisitionThread.cpp Utils.cpp SensorUtils.cpp
LOCAL_MODULE := sensors.$(TARGET_DEVICE)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional

ifneq (,$(filter MPU9150Accelerometer, $(PLATFORM_SENSOR_LIST)))
LOCAL_SRC_FILES += sensors/MPU9150Accelerometer.cpp
endif

ifneq (,$(filter MMA7660Accelerometer, $(PLATFORM_SENSOR_LIST)))
LOCAL_SRC_FILES += sensors/MMA7660Accelerometer.cpp
endif

ifneq (,$(filter LSM9DS0Accelerometer, $(PLATFORM_SENSOR_LIST)))
LOCAL_SRC_FILES += sensors/LSM9DS0Accelerometer.cpp
endif

ifneq (,$(filter LSM303dAccelerometer, $(PLATFORM_SENSOR_LIST)))
LOCAL_SRC_FILES += sensors/LSM303dAccelerometer.cpp
endif

ifneq (,$(filter LSM303dOrientation, $(PLATFORM_SENSOR_LIST)))
LOCAL_SRC_FILES += sensors/LSM303dOrientation.cpp
endif

ifneq (,$(filter GroveLight, $(PLATFORM_SENSOR_LIST)))
LOCAL_SRC_FILES += sensors/GroveLight.cpp
endif

ifneq (,$(filter GroveTemperature, $(PLATFORM_SENSOR_LIST)))
LOCAL_SRC_FILES += sensors/GroveTemperature.cpp
endif

ifneq (,$(filter ProximityGPIO, $(PLATFORM_SENSOR_LIST)))
LOCAL_SRC_FILES += sensors/ProximityGPIO.cpp
endif

include $(BUILD_SHARED_LIBRARY)
