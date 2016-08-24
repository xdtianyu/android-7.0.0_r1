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

#ifndef ANDROID_VEHICLE_HAL_MOCK_H
#define ANDROID_VEHICLE_HAL_MOCK_H

#include <IVehicleNetworkHalMock.h>

extern "C" {
vehicle_prop_config_t const * getTestProperties();
int getNumTestProperties();
};

namespace android {

class VehicleHalMock : public BnVehicleNetworkHalMock {
public:
    VehicleHalMock() {
        mProperties = new VehiclePropertiesHolder(false /* deleteConfigsInDestructor */);
        vehicle_prop_config_t const * properties = getTestProperties();
        for (int i = 0; i < getNumTestProperties(); i++) {
            mProperties->getList().push_back(properties + i);
        }
    };

    virtual ~VehicleHalMock() {};

    virtual sp<VehiclePropertiesHolder> onListProperties() {
        return mProperties;
    };

    virtual status_t onPropertySet(const vehicle_prop_value_t& /*value*/) {
        return NO_ERROR;
    };

    virtual status_t onPropertyGet(vehicle_prop_value_t* /*value*/) {
        return NO_ERROR;
    };

    virtual status_t onPropertySubscribe(int32_t /*property*/, float /*sampleRate*/,
            int32_t /*zones*/) {
        return NO_ERROR;
    };

    virtual void onPropertyUnsubscribe(int32_t /*property*/) {
    };

    bool isTheSameProperties(sp<VehiclePropertiesHolder>& list) {
        if (mProperties->getList().size() != list->getList().size()) {
            return false;
        }
        auto l = mProperties->getList().begin();
        auto r = list->getList().begin();
        while (l != mProperties->getList().end() && r != list->getList().end()) {
            if (!VehiclePropertiesUtil::isTheSame(**l, **r)) {
                return false;
            }
            l++;
            r++;
        }
        return true;
    }

private:
    sp<VehiclePropertiesHolder> mProperties;

};

}; // namespace android
#endif /* ANDROID_VEHICLE_HAL_MOCK_H */
