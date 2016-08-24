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
package android.car.navigation;

import android.annotation.IntDef;
import android.annotation.SystemApi;
import android.os.Parcel;
import android.os.Parcelable;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Holds options related to navigation for the car's instrument cluster.
 * @hide
 */
@SystemApi
public class CarNavigationInstrumentCluster implements Parcelable {

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
            ClusterType.CUSTOM_IMAGES_SUPPORTED,
            ClusterType.IMAGE_CODES_ONLY
    })
    public @interface ClusterType {
        /** Navigation Next Turn messages contain an image, as well as an enum. */
        int CUSTOM_IMAGES_SUPPORTED = 1;
        /** Navigation Next Turn messages only contain an enum. */
        int IMAGE_CODES_ONLY = 2;
    }

    private int mMinIntervalMs;

    @ClusterType
    private int mType;

    private int mImageWidth;

    private int mImageHeight;

    private int mImageColorDepthBits;

    public static final Parcelable.Creator<CarNavigationInstrumentCluster> CREATOR
            = new Parcelable.Creator<CarNavigationInstrumentCluster>() {
        public CarNavigationInstrumentCluster createFromParcel(Parcel in) {
            return new CarNavigationInstrumentCluster(in);
        }

        public CarNavigationInstrumentCluster[] newArray(int size) {
            return new CarNavigationInstrumentCluster[size];
        }
    };

    public static CarNavigationInstrumentCluster createCluster(int minIntervalMs) {
        return new CarNavigationInstrumentCluster(minIntervalMs, ClusterType.IMAGE_CODES_ONLY, 0, 0,
                0);
    }

    public static CarNavigationInstrumentCluster createCustomImageCluster(int minIntervalMs,
            int imageWidth, int imageHeight, int imageColorDepthBits) {
        return new CarNavigationInstrumentCluster(minIntervalMs,
                ClusterType.CUSTOM_IMAGES_SUPPORTED,
                imageWidth, imageHeight, imageColorDepthBits);
    }

    /** Minimum time between instrument cluster updates in milliseconds.*/
    public int getMinIntervalMs() {
        return mMinIntervalMs;
    }

    /** Type of instrument cluster, can be image or enum. */
    @ClusterType
    public int getType() {
        return mType;
    }

    /** If instrument cluster is image, width of instrument cluster in pixels. */
    public int getImageWidth() {
        return mImageWidth;
    }

    /** If instrument cluster is image, height of instrument cluster in pixels. */
    public int getImageHeight() {
        return mImageHeight;
    }

    /** If instrument cluster is image, number of bits of colour depth it supports (8, 16 or 32). */
    public int getImageColorDepthBits() {
        return mImageColorDepthBits;
    }

    public CarNavigationInstrumentCluster(CarNavigationInstrumentCluster that) {
      this(that.mMinIntervalMs,
          that.mType,
          that.mImageWidth,
          that.mImageHeight,
          that.mImageColorDepthBits);
    }

    public boolean supportsCustomImages() {
      return mType == ClusterType.CUSTOM_IMAGES_SUPPORTED;
    }

    private CarNavigationInstrumentCluster(
            int minIntervalMs,
            @ClusterType int type,
            int imageWidth,
            int imageHeight,
            int imageColorDepthBits) {
        this.mMinIntervalMs = minIntervalMs;
        this.mType = type;
        this.mImageWidth = imageWidth;
        this.mImageHeight = imageHeight;
        this.mImageColorDepthBits = imageColorDepthBits;
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mMinIntervalMs);
        dest.writeInt(mType);
        dest.writeInt(mImageWidth);
        dest.writeInt(mImageHeight);
        dest.writeInt(mImageColorDepthBits);
    }

    private CarNavigationInstrumentCluster(Parcel in) {
        mMinIntervalMs = in.readInt();
        mType = in.readInt();
        mImageWidth = in.readInt();
        mImageHeight = in.readInt();
        mImageColorDepthBits = in.readInt();
    }

    /** Converts to string for debug purpose */
    @Override
    public String toString() {
        return CarNavigationInstrumentCluster.class.getSimpleName() + "{ " +
                "minIntervalMs: " + mMinIntervalMs + ", " +
                "type: " + mType + ", " +
                "imageWidth: " + mImageWidth + ", " +
                "imageHeight: " + mImageHeight + ", " +
                "imageColourDepthBits: " + mImageColorDepthBits + " }";
    }
}
