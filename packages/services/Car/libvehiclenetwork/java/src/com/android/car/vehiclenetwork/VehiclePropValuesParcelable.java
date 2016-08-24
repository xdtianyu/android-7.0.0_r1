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

import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValues;
import com.google.protobuf.InvalidProtocolBufferException;

/**
 * Have separate Parcelable for {@link VehiclePropValues} to use only one Blob.
 * Handling this as array of single entry can lead into having multiple Blob,
 * which can increase binder transfer size a lot (as each data can be below blob start size),
 * or can add lots of Blob, which brings lots of performance issue.
 */
public class VehiclePropValuesParcelable implements Parcelable {

    public final VehiclePropValues values;

    public VehiclePropValuesParcelable(VehiclePropValues values) {
        this.values = values;
    }

    public VehiclePropValuesParcelable(Parcel in) {
        byte[] blob = in.readBlob();
        try {
            values = VehiclePropValues.parseFrom(blob);
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
        dest.writeBlob(values.toByteArray());
    }

    public static final Creator<VehiclePropValuesParcelable> CREATOR =
            new Creator<VehiclePropValuesParcelable>() {
        @Override
        public VehiclePropValuesParcelable createFromParcel(Parcel in) {
            return new VehiclePropValuesParcelable(in);
        }

        @Override
        public VehiclePropValuesParcelable[] newArray(int size) {
            return new VehiclePropValuesParcelable[size];
        }
    };
}
