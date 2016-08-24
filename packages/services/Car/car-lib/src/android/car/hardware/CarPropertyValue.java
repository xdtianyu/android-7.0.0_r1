/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.car.hardware;

import static java.lang.Integer.toHexString;

import android.annotation.SystemApi;
import android.os.Parcel;
import android.os.Parcelable;

/**
 * Stores values broken down by area for a vehicle property.
 *
 * @param <T> refer to Parcel#writeValue(Object) to get a list of all supported types. The class
 * should be visible to framework as default class loader is being used here.
 *
 * @hide
 */
@SystemApi
public class CarPropertyValue<T> implements Parcelable {
    private final int mPropertyId;
    private final int mAreaId;
    private final T mValue;

    public CarPropertyValue(int propertyId, T value) {
        this(propertyId, 0, value);
    }

    public CarPropertyValue(int propertyId, int areaId, T value) {
        mPropertyId = propertyId;
        mAreaId = areaId;
        mValue = value;
    }

    @SuppressWarnings("unchecked")
    public CarPropertyValue(Parcel in) {
        mPropertyId = in.readInt();
        mAreaId = in.readInt();
        mValue = (T) in.readValue(getClass().getClassLoader());
    }

    public static final Creator<CarPropertyValue> CREATOR = new Creator<CarPropertyValue>() {
        @Override
        public CarPropertyValue createFromParcel(Parcel in) {
            return new CarPropertyValue(in);
        }

        @Override
        public CarPropertyValue[] newArray(int size) {
            return new CarPropertyValue[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mPropertyId);
        dest.writeInt(mAreaId);
        dest.writeValue(mValue);
    }

    public int getPropertyId() {
        return mPropertyId;
    }

    public int getAreaId() {
        return mAreaId;
    }

    public T getValue() {
        return mValue;
    }

    @Override
    public String toString() {
        return "CarPropertyValue{" +
                "mPropertyId=0x" + toHexString(mPropertyId) +
                ", mAreaId=0x" + toHexString(mAreaId) +
                ", mValue=" + mValue +
                '}';
    }
}
