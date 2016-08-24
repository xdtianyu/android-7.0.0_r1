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
#include "GroveTemperature.hpp"
#include "SensorsHAL.hpp"

struct sensor_t GroveTemperature::sensorDescription = {
  .name = "Grove Temperature Sensor",
  .vendor = "Murata",
  .version = 1,
  .handle = -1,
  .type = SENSOR_TYPE_TEMPERATURE,
  .maxRange = 125.0f,
  .resolution = 1.0f,
  .power = 0.001f,
  .minDelay = 10,
  .fifoReservedEventCount = 0,
  .fifoMaxEventCount = 0,
  .stringType = SENSOR_STRING_TYPE_TEMPERATURE,
  .requiredPermission = "",
  .maxDelay = 1000,
  .flags = SENSOR_FLAG_ON_CHANGE_MODE,
  .reserved = {},
};

Sensor * GroveTemperature::createSensor(int pollFd) {
  return new GroveTemperature(pollFd, 0);
}

void GroveTemperature::initModule() {
  SensorContext::addSensorModule(&sensorDescription, createSensor);
}

GroveTemperature::GroveTemperature(int pollFd, int pin) : upm::GroveTemp(pin), pollFd(pollFd) {
  handle = sensorDescription.handle;
  type = SENSOR_TYPE_TEMPERATURE;
}

GroveTemperature::~GroveTemperature() {}

int GroveTemperature::pollEvents(sensors_event_t* data, int count) {
  data->temperature = value();
  return 1;
}

int GroveTemperature::activate(int handle, int enabled) {
  /* start or stop the acquisition thread */
  return activateAcquisitionThread(pollFd, handle, enabled);
}
