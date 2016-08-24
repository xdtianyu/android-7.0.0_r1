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

#ifndef MPU9150_ACCELEROMETER_HPP
#define MPU9150_ACCELEROMETER_HPP

#include <hardware/sensors.h>
#include "Sensor.hpp"
#include "mpu9150.h"

struct sensors_event_t;

/**
 * MPU9150Accelerometer exposes the MPU9150 accelerometer sensor
 *
 * Overrides the pollEvents & activate Sensor methods.
 */
class MPU9150Accelerometer : public Sensor, public upm::MPU9150 {
  public:
    /**
     * MPU9150Accelerometer constructor
     * @param pollFd poll file descriptor
     * @param bus number of the bus
     * @param address device address
     * @param magAddress magnetometer address
     * @param enableAk8975 whether to enable AK8975 or not
     */
    MPU9150Accelerometer(int pollFd, int bus=MPU9150_I2C_BUS,
        int address=MPU9150_DEFAULT_I2C_ADDR,
        int magAddress=AK8975_DEFAULT_I2C_ADDR, bool enableAk8975=false);

    /**
     * MPU9150Accelerometer destructor
     */
    ~MPU9150Accelerometer() override;

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
    static struct sensor_t sensorDescription;
};

#endif  // MPU9150_ACCELEROMETER_HPP
