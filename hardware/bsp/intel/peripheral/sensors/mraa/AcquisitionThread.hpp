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

#ifndef ACQUISITION_THREAD_HPP
#define ACQUISITION_THREAD_HPP

#include <pthread.h>
#include "Sensor.hpp"

class Sensor;

/**
 * AcquisitionThread is used for implementing sensors polling
 *
 * The class creates a thread to periodically poll data from a
 * sensor and write it to a pipe. The main thread can use the
 * pipe read endpoint to retrieve sensor events.
 *
 * One can also wake up the thread via the wakeup method after
 * changing the sensor parameters.
 *
 * It includes support for generating a flush complete event.
 */
class AcquisitionThread {
  public:
    /**
     * AcquisitionThread constructor
     * @param pollFd poll file descriptor
     * @param sensor the sensor to associate with the thread
     */
    AcquisitionThread(int pollFd, Sensor *sensor);

    /**
     * AcquistionThread destructor
     */
    ~AcquisitionThread();

    /**
     * Get sensor associated with the thread
     * @return the associated sensor
     */
    Sensor * getSensor() { return sensor; }

    /**
     * Get the file descriptor of the pipe read endpoint
     * @return the pipe read file descriptor
     */
    int getReadPipeFd() { return pipeFds[0]; }

    /**
     * Get the file descriptor of the pipe write endpoint
     * @return the pipe write file descriptor
     */
    int getWritePipeFd() { return pipeFds[1]; }

    /**
     * Initialize the acquisition thread
     * @return true if successful, false otherwise
     */
    bool init();

    /**
     * Generate a flush event and send it via the associated pipe
     * @return true if successful, false otherwise
     */
    bool generateFlushCompleteEvent();

    /**
     * Wake up thread if it is sleeping
     * @return 0 if successful, < 0 otherwise
     */
    int wakeup();

  private:
    static void * acquisitionRoutine(void *param);

    int pollFd;
    int pipeFds[2];
    pthread_t pthread;
    pthread_condattr_t pthreadCondAttr;
    pthread_cond_t pthreadCond;
    pthread_mutex_t pthreadMutex;
    Sensor *sensor;
    bool initialized;
};

#endif  // ACQUISITION_THREAD_HPP
