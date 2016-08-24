/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tv.settings.widget.picker;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * Picker column class used by {@link Picker}
 */
public class PickerColumn implements Parcelable {

    private final String[] mItems;

    public PickerColumn(String[] items) {
        if (items == null) {
            throw new IllegalArgumentException("items for PickerColumn cannot be null");
        }
        mItems = items;
    }

    public PickerColumn(Parcel source) {
        int count = source.readInt();
        mItems = new String[count];
        source.readStringArray(mItems);
    }

    public String[] getItems() {
        return mItems;
    }

    public static Parcelable.Creator<PickerColumn>
            CREATOR = new Parcelable.Creator<PickerColumn>() {

                @Override
                public PickerColumn createFromParcel(Parcel source) {
                    return new PickerColumn(source);
                }

                @Override
                public PickerColumn[] newArray(int size) {
                    return new PickerColumn[size];
                }
            };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mItems.length);
        dest.writeStringArray(mItems);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();

        for (String s : mItems) {
            sb.append(s).append("\n");
        }
        return sb.toString();
    }
}
