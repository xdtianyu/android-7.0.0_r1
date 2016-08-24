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

package com.android.usbtuner.tvinput;

import android.os.SystemClock;
import android.util.Log;

/**
 * A class to maintain various debugging information.
 */
public class UsbTunerDebug {
    private static final String TAG = "UsbTunerDebug";
    public static final boolean ENABLED = false;

    private int mVideoFrameDrop;
    private int mBytesInQueue;

    private long mAudioPositionUs;
    private long mAudioPtsUs;
    private long mVideoPtsUs;

    private long mLastAudioPositionUs;
    private long mLastAudioPtsUs;
    private long mLastVideoPtsUs;
    private long mLastCheckTimestampMs;

    private long mAudioPositionUsRate;
    private long mAudioPtsUsRate;
    private long mVideoPtsUsRate;

    private UsbTunerDebug() {
        mVideoFrameDrop = 0;
        mLastCheckTimestampMs = SystemClock.elapsedRealtime();
    }

    private static class LazyHolder {
        private static final UsbTunerDebug INSTANCE = new UsbTunerDebug();
    }

    public static UsbTunerDebug getInstance() {
        return LazyHolder.INSTANCE;
    }

    public static void notifyVideoFrameDrop(long delta) {
        // TODO: provide timestamp mismatch information using delta
        UsbTunerDebug sUsbTunerDebug = getInstance();
        sUsbTunerDebug.mVideoFrameDrop++;
    }

    public static int getVideoFrameDrop() {
        UsbTunerDebug sUsbTunerDebug = getInstance();
        int videoFrameDrop = sUsbTunerDebug.mVideoFrameDrop;
        if (videoFrameDrop > 0) {
            Log.d(TAG, "Dropped video frame: " + videoFrameDrop);
        }
        sUsbTunerDebug.mVideoFrameDrop = 0;
        return videoFrameDrop;
    }

    public static void setBytesInQueue(int bytesInQueue) {
        UsbTunerDebug sUsbTunerDebug = getInstance();
        sUsbTunerDebug.mBytesInQueue = bytesInQueue;
    }

    public static int getBytesInQueue() {
        UsbTunerDebug sUsbTunerDebug = getInstance();
        return sUsbTunerDebug.mBytesInQueue;
    }

    public static void setAudioPositionUs(long audioPositionUs) {
        UsbTunerDebug sUsbTunerDebug = getInstance();
        sUsbTunerDebug.mAudioPositionUs = audioPositionUs;
    }

    public static long getAudioPositionUs() {
        UsbTunerDebug sUsbTunerDebug = getInstance();
        return sUsbTunerDebug.mAudioPositionUs;
    }

    public static void setAudioPtsUs(long audioPtsUs) {
        UsbTunerDebug sUsbTunerDebug = getInstance();
        sUsbTunerDebug.mAudioPtsUs = audioPtsUs;
    }

    public static long getAudioPtsUs() {
        UsbTunerDebug sUsbTunerDebug = getInstance();
        return sUsbTunerDebug.mAudioPtsUs;
    }

    public static void setVideoPtsUs(long videoPtsUs) {
        UsbTunerDebug sUsbTunerDebug = getInstance();
        sUsbTunerDebug.mVideoPtsUs = videoPtsUs;
    }

    public static long getVideoPtsUs() {
        UsbTunerDebug sUsbTunerDebug = getInstance();
        return sUsbTunerDebug.mVideoPtsUs;
    }

    public static void calculateDiff() {
        UsbTunerDebug sUsbTunerDebug = getInstance();
        long currentTime = SystemClock.elapsedRealtime();
        long duration = currentTime - sUsbTunerDebug.mLastCheckTimestampMs;
        if (duration != 0) {
            sUsbTunerDebug.mAudioPositionUsRate =
                    (sUsbTunerDebug.mAudioPositionUs - sUsbTunerDebug.mLastAudioPositionUs) * 1000
                    / duration;
            sUsbTunerDebug.mAudioPtsUsRate =
                    (sUsbTunerDebug.mAudioPtsUs - sUsbTunerDebug.mLastAudioPtsUs) * 1000
                    / duration;
            sUsbTunerDebug.mVideoPtsUsRate =
                    (sUsbTunerDebug.mVideoPtsUs - sUsbTunerDebug.mLastVideoPtsUs) * 1000
                    / duration;
        }

        sUsbTunerDebug.mLastAudioPositionUs = sUsbTunerDebug.mAudioPositionUs;
        sUsbTunerDebug.mLastAudioPtsUs = sUsbTunerDebug.mAudioPtsUs;
        sUsbTunerDebug.mLastVideoPtsUs = sUsbTunerDebug.mVideoPtsUs;
        sUsbTunerDebug.mLastCheckTimestampMs = currentTime;
    }

    public static long getAudioPositionUsRate() {
        UsbTunerDebug sUsbTunerDebug = getInstance();
        return sUsbTunerDebug.mAudioPositionUsRate;
    }

    public static long getAudioPtsUsRate() {
        UsbTunerDebug sUsbTunerDebug = getInstance();
        return sUsbTunerDebug.mAudioPtsUsRate;
    }

    public static long getVideoPtsUsRate() {
        UsbTunerDebug sUsbTunerDebug = getInstance();
        return sUsbTunerDebug.mVideoPtsUsRate;
    }
}
