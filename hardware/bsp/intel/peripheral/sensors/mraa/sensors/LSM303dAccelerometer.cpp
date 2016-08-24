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
#include "LSM303dAccelerometer.hpp"
#include "SensorsHAL.hpp"
#include "SensorUtils.hpp"

struct sensor_t LSM303dAccelerometer::sensorDescription = {
  .name = "LSM303d Accelerometer",
  .vendor = "Unknown",
  .version = 1,
  .handle = -1,
  .type = SENSOR_TYPE_ACCELEROMETER,
  .maxRange = 16.0f,
  .resolution = 0.00003f,
  .power = 0.0003f,
  .minDelay = 0,
  .fifoReservedEventCount = 0,
  .fifoMaxEventCount = 0,
  .stringType = SENSOR_STRING_TYPE_ACCELEROMETER,
  .requiredPermission = "",
  .maxDelay = 0,
  .flags = SENSOR_FLAG_CONTINUOUS_MODE,
  .reserved = {},
};

Sensor * LSM303dAccelerometer::createSensor(int pollFd) {
  return new LSM303dAccelerometer(pollFd, SensorUtils::getI2cBusNumber());
}

void LSM303dAccelerometer::initModule() {
  SensorContext::addSensorModule(&sensorDescription, createSensor);
}

LSM303dAccelerometer::LSM303dAccelerometer(int pollFd,
    int bus, int address, int scale)
    : LSM303d(bus, address, scale), pollFd(pollFd), scale(scale) {
  this->type = SENSOR_TYPE_ACCELEROMETER;
  this->handle = sensorDescription.handle;
}

LSM303dAccelerometer::~LSM303dAccelerometer() {}

/*
 * The raw data from the X,Y,Z axis are expressed in a 16 bit two's complement
 * format.
 *   1. We divide the 16bit value by (2**15) to convert it to floating point +-[0..1]
 *   2. We multiply by the scaling factor to adjust for max range (2,4,6,8,16 G)
 *   3. We multiply by the gravitational accelleration to convert from "g" to m/s**2
 */
int LSM303dAccelerometer::pollEvents(sensors_event_t* data, int count) {
  getAcceleration();
  int16_t *rawdatap = getRawAccelData();
  double conversion_constant = (double)scale * (double)Sensor::kGravitationalAcceleration / pow(2,15);
  data->acceleration.x = (double)rawdatap[0] * conversion_constant;
  data->acceleration.y = (double)rawdatap[1] * conversion_constant;
  data->acceleration.z = (double)rawdatap[2] * conversion_constant;
  return 1;
}

int LSM303dAccelerometer::activate(int handle, int enabled) {
  /* start or stop the acquisition thread */
  return activateAcquisitionThread(pollFd, handle, enabled);
}
