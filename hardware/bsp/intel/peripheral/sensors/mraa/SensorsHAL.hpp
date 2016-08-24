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

#ifndef SENSORS_HAL_HPP
#define SENSORS_HAL_HPP

#include <hardware/sensors.h>
#include <utils/Mutex.h>
#include "Sensor.hpp"
#include "SensorUtils.hpp"

/**
 * Maximum number of sensor devices
 */
#define MAX_DEVICES  20

/**
 * SensorContext represents the HAL entry class
 *
 * The SensorContext class is responsible for initializing
 * a sensors_poll_device_1_t data structure and exposing the
 * sensors.h API methods.
 */
class SensorContext {
  public:
    /**
     * Sensor poll device
     */
    sensors_poll_device_1_t device;

    /**
     * SensorContext constructor
     */
    SensorContext(const hw_module_t* module);

    /**
     * SensorContext destructor
     */
    ~SensorContext();

    /**
     * Add sensor module by sensor description & sensor factory function
     * @param sensorDesc sensor description
     * @param sensorFactoryFunc sensor factory function
     * @return 0 if success, error otherwise
     */
    static int addSensorModule(struct sensor_t *sensorDesc,
        Sensor * (*sensorFactoryFunc)(int pollFd));

    /**
     * Sensors HAL open wrapper function
     * @param module hardware module
     * @param id device identifier
     * @param device where to store the device address
     * @return 0 if success, error otherwise
     */
    static int OpenWrapper(const struct hw_module_t *module,
                            const char* id, struct hw_device_t **device);
    /**
     * Sensors HAL get_sensors_list wrapper function
     * @param module sensors module
     * @param list where to store the list of available sensors
     * @return 0 if success, error otherwise
     */
    static int GetSensorsListWrapper(struct sensors_module_t *module,
                                      struct sensor_t const **list);
  private:
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);
    int batch(int handle, int flags, int64_t period_ns, int64_t timeout);
    int flush(int handle);

    /*
     * The wrapper pass through to the specific instantiation of
     * the SensorContext.
     */
    static int CloseWrapper(hw_device_t *dev);
    static int ActivateWrapper(sensors_poll_device_t *dev, int handle,
                              int enabled);
    static int SetDelayWrapper(sensors_poll_device_t *dev, int handle,
                              int64_t ns);
    static int PollEventsWrapper(sensors_poll_device_t *dev,
                                sensors_event_t *data, int count);
    static int BatchWrapper(sensors_poll_device_1_t *dev, int handle, int flags,
                            int64_t period_ns, int64_t timeout);
    static int FlushWrapper(sensors_poll_device_1_t *dev, int handle);

    /* Poll file descriptor */
    int pollFd;
    /* Array of sensors */
    Sensor * sensors[MAX_DEVICES];

    /* Array of sensor factory functions */
    static Sensor * (*sensorFactoryFuncs[MAX_DEVICES])(int);
    /* Array of sensor descriptions */
    static struct sensor_t sensorDescs[MAX_DEVICES];
    /* Number of registered sensors */
    static int sensorsNum;
    /* Mutex used to synchronize the Sensor Context */
    static android::Mutex mutex;
};

#endif  // SENSORS_HAL_HPP
