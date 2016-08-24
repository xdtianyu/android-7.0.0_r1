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

#ifndef ANDROID_VEHICLE_NETWORK_PROTO_UTIL_H
#define ANDROID_VEHICLE_NETWORK_PROTO_UTIL_H

#include <stdint.h>
#include <sys/types.h>
#include <string.h>

#include <memory>

#include <hardware/vehicle.h>

#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <binder/IMemory.h>
#include <binder/Parcel.h>

#include <VehicleNetworkProto.pb.h>

namespace android {

class VehicleNetworkProtoUtil {
public:
    static status_t toVehiclePropValue(const vehicle_prop_value_t& in,
            VehiclePropValue& out, bool inPlace = false);

    static status_t fromVehiclePropValue(const VehiclePropValue& in, vehicle_prop_value_t& out,
            bool inPlace = false, bool canIgnoreNoData = false);

    static status_t toVehiclePropValues(const List<vehicle_prop_value_t*>& in,
            VehiclePropValues& out);

    static status_t fromVehiclePropValues(const VehiclePropValues& in,
            List<vehicle_prop_value_t*>& out);

    static status_t toVehiclePropConfig(const vehicle_prop_config& in, VehiclePropConfig& out);

    static status_t fromVehiclePropConfig(const VehiclePropConfig& in, vehicle_prop_config& out);

    static status_t toVehiclePropConfigs(List<vehicle_prop_config_t const*> &in,
            VehiclePropConfigs& out);

    static status_t fromVehiclePropConfigs(const VehiclePropConfigs& in,
            List<vehicle_prop_config_t const*>& out);
};

// ----------------------------------------------------------------------------

class WritableBlobHolder {
public:
    Parcel::WritableBlob* blob;

    WritableBlobHolder(Parcel::WritableBlob* aBlob)
        : blob(aBlob) {
    }

    ~WritableBlobHolder() {
        if (blob != NULL) {
            blob->release();
            delete blob;
        }
    }
};

// ----------------------------------------------------------------------------

// duplicated here is Blob is not public.
class ReadableBlobHolder {
public:
    Parcel::ReadableBlob* blob;

    ReadableBlobHolder(Parcel::ReadableBlob* aBlob)
        : blob(aBlob) {
    }

    ~ReadableBlobHolder() {
        if (blob != NULL) {
            blob->release();
            delete blob;
        }
    }
};

class VehiclePropValueBinderUtil {
public:
    static status_t writeToParcel(Parcel& parcel, const vehicle_prop_value_t& value);

    static status_t readFromParcel(const Parcel& parcel, vehicle_prop_value_t* value,
            bool deleteMembers = true, bool canIgnoreNoData = false);
};

}; // namespace android

#endif /* ANDROID_VEHICLE_NETWORK_PROTO_UTIL_H */
