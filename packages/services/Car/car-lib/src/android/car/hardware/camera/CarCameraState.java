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

package android.car.hardware.camera;

import android.annotation.SystemApi;
import android.os.Parcel;
import android.os.Parcelable;

/**
 * CarCameraState object corresponds to a property of the car's Camera system
 * @hide
 */
@SystemApi
public class CarCameraState implements Parcelable {
    private boolean mCameraIsOn;
    private boolean mOverlayIsOn;

    public int describeContents() {
        return 0;
    }

    public void writeToParcel(Parcel out, int flags) {
        out.writeInt(mCameraIsOn ? 1 : 0);
        out.writeInt(mOverlayIsOn ? 1 : 0);
    }

    public static final Parcelable.Creator<CarCameraState> CREATOR
            = new Parcelable.Creator<CarCameraState>() {
        public CarCameraState createFromParcel(Parcel in) {
            return new CarCameraState(in);
        }

        public CarCameraState[] newArray(int size) {
            return new CarCameraState[size];
        }
    };

    private CarCameraState(Parcel in) {
        mCameraIsOn = in.readInt() == 1;
        mOverlayIsOn = in.readInt() == 1;
    }

    /**
     * Copy constructor
     * @param that
     */
    public CarCameraState(CarCameraState that) {
        mCameraIsOn = that.getCameraIsOn();
        mOverlayIsOn = that.getOverlayIsOn();
    }

    /**
     * Constructor
     */
    public CarCameraState(boolean overlayIsOn, boolean cameraIsOn) {
        mCameraIsOn = cameraIsOn;
        mOverlayIsOn = overlayIsOn;
    }

    // Getters.
    public boolean getCameraIsOn()  { return mCameraIsOn;  }
    public boolean getOverlayIsOn() { return mOverlayIsOn; }

    // Setters.
    public void setCameraIsOn(boolean v)  { mCameraIsOn = v; }
    public void setOverlayIsOn(boolean v) { mOverlayIsOn = v; }

    // Printer.
    public String toString() {
        String myString = "mCameraIsOn: "  + mCameraIsOn + " mOverlayIsOn: "  + mOverlayIsOn;
        return myString;
    }

    // Comparator.
    public boolean equals(Object o) {
        if (o instanceof CarCameraState) {
            CarCameraState that = (CarCameraState) o;

            return (that.getCameraIsOn()  == mCameraIsOn &&
                    that.getOverlayIsOn() == mOverlayIsOn);
        }
        return false;
    }
}
