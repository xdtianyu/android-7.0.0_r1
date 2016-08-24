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

#ifndef LSM303d_ACCELEROMETER_HPP
#define LSM303d_ACCELEROMETER_HPP

#include <hardware/sensors.h>
#include "Sensor.hpp"
#include "lsm303d.h"

struct sensors_event_t;

/**
 * LSM303dAccelerometer exposes the LSM303d accelerometer sensor
 *
 * Overrides the pollEvents & activate Sensor methods.
 */
class LSM303dAccelerometer : public Sensor, public upm::LSM303d {
  public:
    /**
     * LSM303dAccelerometer constructor
     * @param pollFd poll file descriptor
     * @param bus number of the bus
     * @param address device address
     * @param magAddress magnetometer address
     * @param scale Sensor sensitivity scaling
     */
    LSM303dAccelerometer(int pollFd,
        int bus=0,
        int address = LSM303d_ADDR,
        int scale = LM303D_SCALE_2G);

    /**
     * LSM303dAccelerometer destructor
     */
    ~LSM303dAccelerometer() override;

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
    static void initModule() __attribute__((constructor));

    int pollFd;
    int scale;
    static struct sensor_t sensorDescription;
};

#endif  // LSM303d_ACCELEROMETER_HPP
