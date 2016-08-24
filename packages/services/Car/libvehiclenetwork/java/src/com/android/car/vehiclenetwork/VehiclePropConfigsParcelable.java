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
package com.android.car.vehiclenetwork;

import android.os.Parcel;
import android.os.Parcelable;

import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfigs;
import com.google.protobuf.InvalidProtocolBufferException;

public class VehiclePropConfigsParcelable implements Parcelable {

    public final VehiclePropConfigs configs;

    public VehiclePropConfigsParcelable(VehiclePropConfigs configs) {
        this.configs = configs;
    }

    public VehiclePropConfigsParcelable(Parcel in) {
        byte[] blob = in.readBlob();
        try {
            configs = VehiclePropConfigs.parseFrom(blob);
        } catch (InvalidProtocolBufferException e) {
            throw new IllegalArgumentException(e);
        }
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        // this is never used for java side as service is implemented in native side.
        dest.writeBlob(configs.toByteArray());
    }

    public static final Creator<VehiclePropConfigsParcelable> CREATOR =
            new Creator<VehiclePropConfigsParcelable>() {
        @Override
        public VehiclePropConfigsParcelable createFromParcel(Parcel in) {
            return new VehiclePropConfigsParcelable(in);
        }

        @Override
        public VehiclePropConfigsParcelable[] newArray(int size) {
            return new VehiclePropConfigsParcelable[size];
        }
    };
}
