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

#ifndef SENSOR_HPP
#define SENSOR_HPP

/**
 * The default sensor __attribute__((constructor)) priority is represented by
 * the first available priority value. The [0, 100] ones are used by the system
 * implementation.
 */
#define DEFAULT_SENSOR_CONSTRUCTOR_PRIORITY 101

#include "AcquisitionThread.hpp"

struct sensors_event_t;
class AcquisitionThread;

/**
 * Sensor representation class
 *
 * It supports sensor enabling/disabling, changing the sensor's parameters
 * and event reading.
 */
class Sensor {
  public:
    /**
     * Sensor constructor
     */
    Sensor();

    /**
     * Sensor destructor
     */
    virtual ~Sensor();

    /**
     * Activate the sensor
     * @param handle sensor identifier
     * @param enabled 1 for enabling and 0 for disabling
     * @return 0 on success and a negative error number otherwise
     */
    virtual int activate(int handle, int enabled);

    /**
     * Set delay
     * @param handle sensor identifier
     * @param ns the sampling period at which the sensor should run, in nanoseconds
     * @return 0 on success and a negative error number otherwise
     */
    virtual int setDelay(int handle, int64_t ns);

    /**
     * Poll for events
     * @param data where to store the events
     * @param count the number of events returned must be <= to the count
     * @return number of events returned in data on success and a negative error number otherwise
     */
    virtual int pollEvents(sensors_event_t* data, int count) = 0;

    /**
     * Sets a sensor’s parameters, including sampling frequency and maximum report latency
     * @param handle sensor identifier
     * @param flags currently unused
     * @param period_ns the sampling period at which the sensor should run, in nanoseconds
     * @param timeout the maximum time by which events can be delayed before being reported
     *          through the HAL, in nanoseconds
     * @return 0 on success and a negative error number otherwise
     */
    virtual int batch(int handle, int flags, int64_t period_ns, int64_t timeout);

    /**
     * Add a flush complete event to the end of the hardware FIFO for the specified sensor and flushes the FIFO
     * @param handle sensor identifier
     * @return 0 on success and a negative error number otherwise
     */
    virtual int flush(int handle);

    /**
     * Read and store an event
     * @param event where to store the event
     * @return true on success and a false otherwise
     */
    virtual bool readOneEvent(sensors_event_t *event);

    /**
     * Get sensor identifier
     * @return sensor handle
     */
    int getHandle() { return handle; }

    /**
     * Get sensor type
     * @return sensor type
     */
    int getType() { return type; }

    /**
     * Get sensor delay in nanoseconds
     * @return sensor delay
     */
    int64_t getDelay() { return delay; }

    /**
     * Gravitational acceleration constant in m/s^2
     */
    static const float kGravitationalAcceleration;

  protected:
    /**
     * Enable or disable the associated acquisition thread
     * @param pollFd poll file descriptor
     * @param handle sensor identifier
     * @param enabled 1 for enabling and 0 for disabling
     * @return 0 on success and a negative error number otherwise
     */
    virtual int activateAcquisitionThread(int pollFd, int handle, int enabled);

    AcquisitionThread *acquisitionThread;
    int handle, type;
    int64_t delay;
};

#endif  // SENSOR_HPP
