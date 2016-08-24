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

#ifndef ANDROID_IVEHICLE_NETWORK_H
#define ANDROID_IVEHICLE_NETWORK_H

#include <stdint.h>
#include <sys/types.h>

#include <hardware/vehicle.h>

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <binder/IMemory.h>
#include <binder/Parcel.h>

#include <VehicleNetworkDataTypes.h>
#include <IVehicleNetworkHalMock.h>
#include <IVehicleNetworkListener.h>

namespace android {

// ----------------------------------------------------------------------------

class IVehicleNetwork : public IInterface {
public:
    static const char SERVICE_NAME[];
    DECLARE_META_INTERFACE(VehicleNetwork);

    /**
     * Return configuration of single property (when argument property is not 0) or all properties
     * (when property = 0).
     */
    virtual sp<VehiclePropertiesHolder> listProperties(int32_t property = 0) = 0;
    virtual status_t setProperty(const vehicle_prop_value_t& value)= 0;
    virtual status_t getProperty(vehicle_prop_value_t* value) = 0;
    virtual status_t subscribe(const sp<IVehicleNetworkListener> &listener, int32_t property,
            float sampleRate, int32_t zones) = 0;
    virtual void unsubscribe(const sp<IVehicleNetworkListener> &listener, int32_t property) = 0;
    /**
     * Inject event for given property. This should work regardless of mocking but usually
     * used in mocking.
     */
    virtual status_t injectEvent(const vehicle_prop_value_t& value) = 0;
    virtual status_t startMocking(const sp<IVehicleNetworkHalMock>& mock) = 0;
    virtual void stopMocking(const sp<IVehicleNetworkHalMock>& mock) = 0;
    virtual status_t injectHalError(int32_t errorCode, int32_t property, int32_t operation) = 0;
    /**
     * Register listener and listen for global error from vehicle hal.
     * Per property error will be delivered when the property is subscribed or global error listener
     * where there is no subscription.
     */
    virtual status_t startErrorListening(const sp<IVehicleNetworkListener> &listener) = 0;
    virtual void stopErrorListening(const sp<IVehicleNetworkListener> &listener) = 0;
    /**
     * Listen for HAL restart. When HAL restarts, as in case of starting or stopping mocking,
     * all existing subscription becomes invalid.
     */
    virtual status_t startHalRestartMonitoring(const sp<IVehicleNetworkListener> &listener) = 0;
    virtual void stopHalRestartMonitoring(const sp<IVehicleNetworkListener> &listener) = 0;
};

// ----------------------------------------------------------------------------

class BnVehicleNetwork : public BnInterface<IVehicleNetwork> {
    virtual status_t  onTransact(uint32_t code,
                                 const Parcel& data,
                                 Parcel* reply,
                                 uint32_t flags = 0);
    virtual bool isOperationAllowed(int32_t property, bool isWrite) = 0;
    virtual void releaseMemoryFromGet(vehicle_prop_value_t* value) = 0;
};

// ----------------------------------------------------------------------------

class VehicleNetworkUtil {
public:
    static int countNumberOfZones(int32_t zones) {
        if (zones == 0) { // no-zone, treat as one zone.
            return 1;
        }
        int count = 0;
        uint32_t flag = 0x1;
        for (int i = 0; i < 32; i++) {
            if ((flag & zones) != 0) {
                count++;
            }
            flag <<= 1;
        }
        return count;
    }
};

// ----------------------------------------------------------------------------
}; // namespace android

#endif /* ANDROID_IVEHICLE_NETWORK_H */
