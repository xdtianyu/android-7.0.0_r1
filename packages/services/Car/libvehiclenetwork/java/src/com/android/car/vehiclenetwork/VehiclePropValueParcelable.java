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
import android.os.Parcelable.Creator;

import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;
import com.google.protobuf.InvalidProtocolBufferException;

public class VehiclePropValueParcelable implements Parcelable {

    public final VehiclePropValue value;

    public VehiclePropValueParcelable(VehiclePropValue value) {
        this.value = value;
    }

    public VehiclePropValueParcelable(Parcel in) {
        byte[] blob = in.readBlob();
        try {
            value = VehiclePropValue.parseFrom(blob);
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
        dest.writeBlob(value.toByteArray());
    }

    public static final Creator<VehiclePropValueParcelable> CREATOR =
            new Creator<VehiclePropValueParcelable>() {
        @Override
        public VehiclePropValueParcelable createFromParcel(Parcel in) {
            return new VehiclePropValueParcelable(in);
        }

        @Override
        public VehiclePropValueParcelable[] newArray(int size) {
            return new VehiclePropValueParcelable[size];
        }
    };
}
