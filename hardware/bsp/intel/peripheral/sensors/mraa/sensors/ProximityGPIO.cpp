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

/*
 * A Proximity sensor with a true/false value read in via GPIO
 * is implemented. The Sensor HAL returns a float for the proximity
 * distance... but in the case of a GPIO sensor, we only have close and
 * no-close (1/0). We fake the distance by returning the values below.
 */
#define kProximityClose   0.0
#define kProximityFar   100.0


/*
 * We monitor GPIO48 for the proximity reading.
 * This corresponids to IO7 on the Arduino shield and is
 * not multiplexed with any other functionality.
 *
 * The mraa library expects the Arudino sheild pin number here when
 * talking to the Arduino expansion board. Change this appropriate when
 * using a different GPIO pin or different Edison breakout board.
 */
#define kPinGPIO 7

#include <cutils/log.h>
#include "ProximityGPIO.hpp"
#include "SensorsHAL.hpp"

struct sensor_t ProximityGPIO::sensorDescription = {
  .name = "Proximity GPIO Sensor",
  .vendor = "Unknown",
  .version = 1,
  .handle = -1,
  .type = SENSOR_TYPE_PROXIMITY,
  .maxRange = 100.0f,
  .resolution = 1.0f,
  .power = 0.001f,
  .minDelay = 10,
  .fifoReservedEventCount = 0,
  .fifoMaxEventCount = 0,
  .stringType = SENSOR_STRING_TYPE_PROXIMITY,
  .requiredPermission = "",
  .maxDelay = 1000,
  .flags = SENSOR_FLAG_ON_CHANGE_MODE,
  .reserved = {},
};

Sensor * ProximityGPIO::createSensor(int pollFd) {
  return new ProximityGPIO(pollFd, kPinGPIO);
}

void ProximityGPIO::initModule() {
  SensorContext::addSensorModule(&sensorDescription, createSensor);
}

ProximityGPIO::ProximityGPIO(int pollFd, int pin) : upm::GroveButton(pin), pollFd(pollFd) {
  handle = sensorDescription.handle;
  type = SENSOR_TYPE_PROXIMITY;
}

ProximityGPIO::~ProximityGPIO() {}

int ProximityGPIO::pollEvents(sensors_event_t* data, int count) {
  data->distance = !value() ? kProximityClose: kProximityFar;
  return 1;
}

int ProximityGPIO::activate(int handle, int enabled) {
  /* start or stop the acquisition thread */
  return activateAcquisitionThread(pollFd, handle, enabled);
}
