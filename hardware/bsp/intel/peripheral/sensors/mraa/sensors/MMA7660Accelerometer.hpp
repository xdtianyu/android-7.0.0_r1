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

#ifndef MMA7660_ACCELEROMETER_HPP
#define MMA7660_ACCELEROMETER_HPP

#include <hardware/sensors.h>
#include "Sensor.hpp"
#include "mma7660.h"

struct sensors_event_t;

/**
 * MMA7660Accelerometer exposes the MMA7660 accelerometer sensor
 *
 * Overrides the pollEvents & activate Sensor methods.
 */
class MMA7660Accelerometer : public Sensor, public upm::MMA7660 {
  public:
    static const int kMaxRange = 1000;
    /**
     * Time period in microseconds (1/64 * 10^6 = 15625) to wait before
     * requesting events for the default activation sampling rate (64 Hz)
     */
    static const int kActivationPeriod = 15625;

    /**
     * MMA7660Accelerometer constructor
     * @param pollFd poll file descriptor
     * @param bus number of the bus
     * @param address device address
     */
    MMA7660Accelerometer(int pollFd, int bus, uint8_t address);

    /**
     * MMA7660Accelerometer destructor
     */
    ~MMA7660Accelerometer() override;

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

#endif  // MMA7660_ACCELEROMETER_HPP
