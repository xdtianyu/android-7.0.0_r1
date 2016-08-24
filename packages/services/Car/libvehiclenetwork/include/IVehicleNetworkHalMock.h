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

#ifndef ANDROID_IVEHICLE_NETWORK_HAL_MOCK_H
#define ANDROID_IVEHICLE_NETWORK_HAL_MOCK_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <binder/IMemory.h>
#include <binder/Parcel.h>

#include <VehicleNetworkDataTypes.h>

namespace android {

// ----------------------------------------------------------------------------

class IVehicleNetworkHalMock : public IInterface
{
public:
    static const char SERVICE_NAME[];
    DECLARE_META_INTERFACE(VehicleNetworkHalMock);

    virtual sp<VehiclePropertiesHolder> onListProperties() = 0;
    virtual status_t onPropertySet(const vehicle_prop_value_t& value) = 0;
    virtual status_t onPropertyGet(vehicle_prop_value_t* value) = 0;
    virtual status_t onPropertySubscribe(int32_t property, float sampleRate, int32_t zones) = 0;
    virtual void onPropertyUnsubscribe(int32_t property) = 0;
};

// ----------------------------------------------------------------------------

class BnVehicleNetworkHalMock : public BnInterface<IVehicleNetworkHalMock>
{
    virtual status_t  onTransact(uint32_t code,
                                 const Parcel& data,
                                 Parcel* reply,
                                 uint32_t flags = 0);
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif /* ANDROID_IVEHICLE_NETWORK_HAL_MOCK_H */
