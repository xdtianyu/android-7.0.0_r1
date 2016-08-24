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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or impliedt data output.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cutils/log.h>
#include "LSM9DS0Accelerometer.hpp"
#include "SensorsHAL.hpp"
#include "SensorUtils.hpp"

struct sensor_t LSM9DS0Accelerometer::sensorDescription = {
  .name = "LSM9DS0 Accelerometer",
  .vendor = "Unknown",
  .version = 1,
  .handle = -1,
  .type = SENSOR_TYPE_ACCELEROMETER,
  .maxRange = 16.0f,
  .resolution = 0.061,
  .power = 0.000350,
  .minDelay = 0,
  .fifoReservedEventCount = 0,
  .fifoMaxEventCount = 0,
  .stringType = SENSOR_STRING_TYPE_ACCELEROMETER,
  .requiredPermission = "",
  .maxDelay = 1000,
  .flags = SENSOR_FLAG_CONTINUOUS_MODE,
  .reserved = {},
};

Sensor * LSM9DS0Accelerometer::createSensor(int pollFd) {
  return new LSM9DS0Accelerometer(pollFd,
      SensorUtils::getI2cBusNumber(),
      LSM9DS0_DEFAULT_GYRO_ADDR, LSM9DS0_DEFAULT_XM_ADDR);

}

void LSM9DS0Accelerometer::initModule() {
  SensorContext::addSensorModule(&sensorDescription, createSensor);
}

LSM9DS0Accelerometer::LSM9DS0Accelerometer(int pollFd,
    int bus, uint8_t gAddress, uint8_t xmAddress)
    : LSM9DS0 (bus, gAddress, xmAddress), pollFd(pollFd) {
  handle = sensorDescription.handle;
  type = SENSOR_TYPE_ACCELEROMETER;
}

LSM9DS0Accelerometer::~LSM9DS0Accelerometer() {}

int LSM9DS0Accelerometer::pollEvents(sensors_event_t* data, int count) {
  updateAccelerometer();
  getAccelerometer(&data->acceleration.x, &data->acceleration.y, &data->acceleration.z);
  data->acceleration.x *= Sensor::kGravitationalAcceleration;
  data->acceleration.y *= Sensor::kGravitationalAcceleration;
  data->acceleration.z *= Sensor::kGravitationalAcceleration;
  return 1;
}

// LSM9DS0 accelerometer sensor implementation
int LSM9DS0Accelerometer::activate(int handle, int enabled) {
  init();

  /* start or stop the acquisition thread */
  return activateAcquisitionThread(pollFd, handle, enabled);
}
