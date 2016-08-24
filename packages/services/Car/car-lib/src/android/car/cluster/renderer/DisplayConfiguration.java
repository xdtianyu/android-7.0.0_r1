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
package android.car.cluster.renderer;

import android.annotation.Nullable;
import android.annotation.SystemApi;
import android.graphics.Rect;
import android.os.Parcel;
import android.os.Parcelable;

/**
 * TODO: need to properly define this class and make it immutable.
 *
 * @hide
 */
@SystemApi
public class DisplayConfiguration implements Parcelable {
    private final Rect mPrimaryRegion;

    @Nullable
    private final Rect mSecondaryRegion;

    public static final Parcelable.Creator<DisplayConfiguration> CREATOR =
            new Parcelable.Creator<DisplayConfiguration>() {

                public DisplayConfiguration createFromParcel(Parcel in) {
                    return new DisplayConfiguration(in);
                }

                public DisplayConfiguration[] newArray(int size) {
                    return new DisplayConfiguration[size];
                }
            };


    public DisplayConfiguration(Parcel in) {
        mPrimaryRegion = in.readTypedObject(Rect.CREATOR);
        mSecondaryRegion = in.readTypedObject(Rect.CREATOR);
    }

    public DisplayConfiguration(Rect primaryRegion) {
        this(primaryRegion, null);
    }

    public DisplayConfiguration(Rect primaryRegion, @Nullable Rect secondaryRegion) {
        mPrimaryRegion = primaryRegion;
        mSecondaryRegion = secondaryRegion;
    }


    /** Region that will be fully visible in instrument cluster */
    public Rect getPrimaryRegion() {
        return mPrimaryRegion;
    }

    /**
     * The region that includes primary region + may include some additional space that might
     * be partially visible in the instrument cluster. It is useful to fade-out primary
     * rendering for example or adding background image.
     */
    @Nullable
    public Rect getSecondaryRegion() {
        return mSecondaryRegion;
    }

    /** Returns true if secondary region is strongly greater then primary region */
    public boolean hasSecondaryRegion() {
        return mSecondaryRegion != null
                && mSecondaryRegion.width() > mPrimaryRegion.width()
                && mSecondaryRegion.height() > mPrimaryRegion.height();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeTypedObject(mPrimaryRegion, 0);
        dest.writeTypedObject(mSecondaryRegion, 0);
    }
}
