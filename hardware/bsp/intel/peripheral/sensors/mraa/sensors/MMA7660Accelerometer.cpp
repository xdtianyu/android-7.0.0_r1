/*
 * Copyright (C) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cutils/log.h>
#include "MMA7660Accelerometer.hpp"
#include "SensorsHAL.hpp"
#include "SensorUtils.hpp"

struct sensor_t MMA7660Accelerometer::sensorDescription = {
  .name = "MMA7660 Accelerometer",
  .vendor = "Freescale Semiconductor",
  .version = 1,
  .handle = -1,
  .type = SENSOR_TYPE_ACCELEROMETER,
  /* maxRange = 1.5g */
  .maxRange = 14.72f,
  .resolution = 0.459915612f,
  .power = 0.047f,
  .minDelay = 10,
  .fifoReservedEventCount = 0,
  .fifoMaxEventCount = 0,
  .stringType = SENSOR_STRING_TYPE_ACCELEROMETER,
  .requiredPermission = "",
  .maxDelay = 1000,
  .flags = SENSOR_FLAG_CONTINUOUS_MODE,
  .reserved = {},
};

Sensor * MMA7660Accelerometer::createSensor(int pollFd) {
  return new MMA7660Accelerometer(pollFd,
      SensorUtils::getI2cBusNumber(),
      MMA7660_DEFAULT_I2C_ADDR);
}

void MMA7660Accelerometer::initModule() {
  SensorContext::addSensorModule(&sensorDescription, createSensor);
}

MMA7660Accelerometer::MMA7660Accelerometer(int pollFd, int bus, uint8_t address)
    : MMA7660 (bus, address), pollFd(pollFd) {
  handle = sensorDescription.handle;
  type = SENSOR_TYPE_ACCELEROMETER;
}

MMA7660Accelerometer::~MMA7660Accelerometer() {}

int MMA7660Accelerometer::pollEvents(sensors_event_t* data, int count) {
  getAcceleration(&data->acceleration.x, &data->acceleration.y, &data->acceleration.z);
  data->acceleration.x *= Sensor::kGravitationalAcceleration;
  data->acceleration.y *= Sensor::kGravitationalAcceleration;
  data->acceleration.z *= Sensor::kGravitationalAcceleration;
  return 1;
}

// MMA7660 accelerometer sensor implementation
int MMA7660Accelerometer::activate(int handle, int enabled) {
  setModeStandby();
  if (enabled) {
    if (!setSampleRate(upm::MMA7660::AUTOSLEEP_64)) {
      ALOGE("%s: Failed to set sensor SampleRate", __func__);
      return -1;
    }

    setModeActive();
    usleep(kActivationPeriod);
  }

  /* start or stop the acquisition thread */
  return activateAcquisitionThread(pollFd, handle, enabled);
}
