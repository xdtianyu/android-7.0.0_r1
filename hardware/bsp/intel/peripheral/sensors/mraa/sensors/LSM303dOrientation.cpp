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
#include "LSM303dOrientation.hpp"
#include "SensorsHAL.hpp"
#include "SensorUtils.hpp"

struct sensor_t LSM303dOrientation::sensorDescription = {
  .name = "LSM303d Orientation",
  .vendor = "Unknown",
  .version = 1,
  .handle = -1,
  .type = SENSOR_TYPE_ORIENTATION,
  .maxRange = 12.0f,
  .resolution = .00003f,
  .power = 0.0003f,
  .minDelay = 0,
  .fifoReservedEventCount = 0,
  .fifoMaxEventCount = 0,
  .stringType = SENSOR_STRING_TYPE_ORIENTATION,
  .requiredPermission = "",
  .maxDelay = 0,
  .flags = SENSOR_FLAG_CONTINUOUS_MODE,
  .reserved = {},
};

Sensor * LSM303dOrientation::createSensor(int pollFd) {
  return new LSM303dOrientation(pollFd, SensorUtils::getI2cBusNumber());
}

void LSM303dOrientation::initModule() {
  SensorContext::addSensorModule(&sensorDescription, createSensor);
}

LSM303dOrientation::LSM303dOrientation(int pollFd,
    int bus, int address)
    : LSM303d(bus, address), pollFd(pollFd) {
  this->type = SENSOR_TYPE_ORIENTATION;
  this->handle = sensorDescription.handle;
}

LSM303dOrientation::~LSM303dOrientation() {}

int LSM303dOrientation::pollEvents(sensors_event_t* data, int count) {
  getCoordinates();
  int16_t *rawdatap = getRawCoorData();
  data->orientation.x = (double)rawdatap[0];
  data->orientation.y = (double)rawdatap[1];
  data->orientation.z = (double)rawdatap[2];
  return 1;
}

int LSM303dOrientation::activate(int handle, int enabled) {
  /* start or stop the acquisition thread */
  return activateAcquisitionThread(pollFd, handle, enabled);
}
