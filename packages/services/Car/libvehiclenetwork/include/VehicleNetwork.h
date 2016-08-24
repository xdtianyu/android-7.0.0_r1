/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef ANDROID_VEHICLE_NETWORK_H
#define ANDROID_VEHICLE_NETWORK_H

#include <stdint.h>
#include <sys/types.h>

#include <binder/IInterface.h>
#include <binder/IMemory.h>

#include <utils/threads.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/RefBase.h>

#include "IVehicleNetwork.h"
#include "HandlerThread.h"

namespace android {

// ----------------------------------------------------------------------------

/**
 * Listener for client to implement to get events from Vehicle network service.
 */
class VehicleNetworkListener : public RefBase
{
public:
    VehicleNetworkListener() {};
    virtual ~VehicleNetworkListener() {};
    virtual void onEvents(sp<VehiclePropValueListHolder>& events) = 0;
    virtual void onHalError(int32_t errorCode, int32_t property, int32_t operation) = 0;
    virtual void onHalRestart(bool inMocking) = 0;
};

// ----------------------------------------------------------------------------

/** For internal event handling, not for client */
class VehicleNetworkEventMessageHandler : public MessageHandler {
    enum {
        EVENT_EVENTS = 0,
        EVENT_HAL_ERROR = 1,
        EVENT_HAL_RESTART = 2,
    };
public:
    VehicleNetworkEventMessageHandler(const sp<Looper>& looper,
            sp<VehicleNetworkListener>& listener);
    virtual ~VehicleNetworkEventMessageHandler();

    void handleHalEvents(sp<VehiclePropValueListHolder>& events);
    void handleHalError(int32_t errorCode, int32_t property, int32_t operation);
    /**
     * This error must be handled always. This can be called in vehicle network service's crash
     * as well.
     */
    void handleHalRestart(bool inMocking);

private:
    virtual void handleMessage(const Message& message);
    void doHandleHalEvents();
    void doHandleHalError();
    void doHandleHalRestart();
private:
    mutable Mutex mLock;
    sp<Looper> mLooper;
    sp<VehicleNetworkListener>& mListener;
    List<sp<VehiclePropValueListHolder>> mEvents;
    List<VehicleHalError*> mHalErrors;
    List<bool> mHalRestartEvents;
};

// ----------------------------------------------------------------------------

/**
 * Vehicle network API for low level components like HALs to access / control car information.
 * This is reference counted. So use with sp<>.
 */
class VehicleNetwork : public IBinder::DeathRecipient, public BnVehicleNetworkListener
{
public:
    /**
     * Factory method for VehicleNetwork. Client should use this method to create
     * a new instance.
     */
    static sp<VehicleNetwork> createVehicleNetwork(sp<VehicleNetworkListener> &listener);

    virtual ~VehicleNetwork();

    /** Set int32 value */
    status_t setInt32Property(int32_t property, int32_t value);
    /** get int32 value */
    status_t getInt32Property(int32_t property, int32_t* value, int64_t* timestamp);
    status_t setInt64Property(int32_t property, int64_t value);
    status_t getInt64Property(int32_t property, int64_t* value, int64_t* timestamp);
    status_t setFloatProperty(int32_t property, float value);
    status_t getFloatProperty(int32_t property, float* value, int64_t* timestamp);
    status_t setStringProperty(int32_t property, const String8& value);
    status_t getStringProperty(int32_t property, String8& value, int64_t* timestamp);
    sp<VehiclePropertiesHolder> listProperties(int32_t property = 0);
    /** For generic value setting. At least prop, value_type, and value should be set. */
    status_t setProperty(const vehicle_prop_value_t& value);
    /** For generic value getting. value->prop should be set. */
    status_t getProperty(vehicle_prop_value_t* value);
    status_t subscribe(int32_t property, float sampleRate, int32_t zones = 0);
    void unsubscribe(int32_t property);

    // Only for testing purpose
    status_t injectEvent(const vehicle_prop_value_t& value);

    // starting / stopping mocking not added yet.
    status_t startMocking(const sp<IVehicleNetworkHalMock>& mock);
    void stopMocking(const sp<IVehicleNetworkHalMock>& mock);

    // only for testing
    status_t injectHalError(int32_t errorCode, int32_t property, int32_t operation);

    status_t startErrorListening();
    void stopErrorListening();

    //IBinder::DeathRecipient, not for client
    void binderDied(const wp<IBinder>& who);
    // BnVehicleNetworkListener, not for client
    void onEvents(sp<VehiclePropValueListHolder>& events);
    void onHalError(int32_t errorCode, int32_t property, int32_t operation);
    void onHalRestart(bool inMocking);

private:
    VehicleNetwork(sp<IVehicleNetwork>& vehicleNetwork, sp<VehicleNetworkListener> &listener);
    // RefBase
    virtual void onFirstRef();
    sp<IVehicleNetwork> getService();
    sp<VehicleNetworkEventMessageHandler> getEventHandler();

private:
    sp<IVehicleNetwork> mService;
    sp<VehicleNetworkListener> mClientListener;
    Mutex mLock;
    sp<HandlerThread> mHandlerThread;
    sp<VehicleNetworkEventMessageHandler> mEventHandler;
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif /* ANDROID_VEHICLE_NETWORK_H */

