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
#include "MPU9150Accelerometer.hpp"
#include "SensorsHAL.hpp"
#include "SensorUtils.hpp"

struct sensor_t MPU9150Accelerometer::sensorDescription = {
  .name = "MPU9150/9250 Accelerometer",
  .vendor = "InvenSense",
  .version = 1,
  .handle = -1,
  .type = SENSOR_TYPE_ACCELEROMETER,
  /* maxRange = 2g */
  .maxRange = 19.62f,
  .resolution = 0.000061035f,
  .power = 0.0198f,
  .minDelay = 10,
  .fifoReservedEventCount = 0,
  .fifoMaxEventCount = 0,
  .stringType = SENSOR_STRING_TYPE_ACCELEROMETER,
  .requiredPermission = "",
  .maxDelay = 1000,
  .flags = SENSOR_FLAG_CONTINUOUS_MODE,
  .reserved = {},
};

Sensor * MPU9150Accelerometer::createSensor(int pollFd) {
  return new MPU9150Accelerometer(pollFd,
      SensorUtils::getI2cBusNumber(),
      MPU9150_DEFAULT_I2C_ADDR, AK8975_DEFAULT_I2C_ADDR, false);
}

void MPU9150Accelerometer::initModule() {
  SensorContext::addSensorModule(&sensorDescription, createSensor);
}

MPU9150Accelerometer::MPU9150Accelerometer(int pollFd, int bus, int address,
    int magAddress, bool enableAk8975)
    : MPU9150(bus, address, magAddress, enableAk8975), pollFd(pollFd) {
  handle = sensorDescription.handle;
  type = SENSOR_TYPE_ACCELEROMETER;
}

MPU9150Accelerometer::~MPU9150Accelerometer() {}

int MPU9150Accelerometer::pollEvents(sensors_event_t* data, int count) {
  update();
  getAccelerometer(&data->acceleration.x, &data->acceleration.y, &data->acceleration.z);
  data->acceleration.x *= Sensor::kGravitationalAcceleration;
  data->acceleration.y *= Sensor::kGravitationalAcceleration;
  data->acceleration.z *= Sensor::kGravitationalAcceleration;
  return 1;
}

int MPU9150Accelerometer::activate(int handle, int enabled) {
  if (enabled) {
    if (init() != true) {
      ALOGE("%s: Failed to initialize sensor error", __func__);
      return -1;
    }
  }

  /* start or stop the acquisition thread */
  return activateAcquisitionThread(pollFd, handle, enabled);
}
