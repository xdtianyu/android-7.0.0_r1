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

#ifndef PROXIMITY_GPIO_HPP
#define PROXIMITY_GPIO_HPP

#include <hardware/sensors.h>
#include "Sensor.hpp"
#include "grove.h"

struct sensors_event_t;

/**
 * ProximityGPIO exposes the Grove Button sensor
 *
 * Overrides the pollEvents & activate Sensor methods.
 */
class ProximityGPIO : public Sensor, public upm::GroveButton {
  public:
    /**
     * GroveButton constructor
     * @param pollFd poll file descriptor
     * @param pin number of the analog input
     */
    ProximityGPIO(int pollFd, int pin);

    /**
     * ProximityGPIO destructor
     */
    ~ProximityGPIO() override;

    /**
     * Poll for events
     * @param data where to store the events
     * @param count the number of events returned must be <= to the count
     * @return number of events returned in data on success and a negative error number otherwise
     */
    int pollEvents(sensors_event_t* data, int count) override;

    /**
     * Activate the sensor
     * @param handle sensor identifier
     * @param enabled 1 for enabling and 0 for disabling
     * @return 0 on success and a negative error number otherwise
     */
    int activate(int handle, int enabled);

  private:
    static Sensor * createSensor(int pollFd);
    static void initModule() __attribute__((constructor
        (DEFAULT_SENSOR_CONSTRUCTOR_PRIORITY)));

    int pollFd;
    static struct sensor_t sensorDescription;
};

#endif  // PROXIMITY_GPIO_HPP
