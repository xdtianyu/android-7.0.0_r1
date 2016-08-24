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

package com.android.usbtuner;

import android.content.Context;
import android.support.annotation.IntDef;
import android.support.annotation.StringDef;
import android.util.Log;

import com.android.usbtuner.util.TisConfiguration;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Objects;

/**
 * A base class to handle a hardware tuner device.
 */
public abstract class TunerHal implements AutoCloseable {
    protected static final String TAG = "TunerHal";
    protected static final boolean DEBUG = false;

    @IntDef({ FILTER_TYPE_OTHER, FILTER_TYPE_AUDIO, FILTER_TYPE_VIDEO, FILTER_TYPE_PCR })
    @Retention(RetentionPolicy.SOURCE)
    public @interface FilterType {}
    public static final int FILTER_TYPE_OTHER = 0;
    public static final int FILTER_TYPE_AUDIO = 1;
    public static final int FILTER_TYPE_VIDEO = 2;
    public static final int FILTER_TYPE_PCR = 3;

    @StringDef({ MODULATION_8VSB, MODULATION_QAM256 })
    @Retention(RetentionPolicy.SOURCE)
    public @interface ModulationType {}
    public static final String MODULATION_8VSB = "8VSB";
    public static final String MODULATION_QAM256 = "QAM256";

    protected static final int PID_PAT = 0;
    protected static final int PID_ATSC_SI_BASE = 0x1ffb;
    protected static final int DEFAULT_VSB_TUNE_TIMEOUT_MS = 2000;
    protected static final int DEFAULT_QAM_TUNE_TIMEOUT_MS = 4000; // Some device takes time for
                                                                   // QAM256 tuning.
    private boolean mIsStreaming;
    private int mFrequency;
    private String mModulation;

    static {
        System.loadLibrary("tunertvinput_jni");
    }

    /**
     * Creates a TunerHal instance.
     * @param context context for creating the TunerHal instance
     * @return the TunerHal instance
     */
    public static TunerHal createInstance(Context context) {
        TunerHal tunerHal;
        if (TisConfiguration.isPackagedWithLiveChannels(context)) {
            tunerHal = new UsbTunerHal(context);
        } else {
            tunerHal = new InternalTunerHal(context);
        }
        if (tunerHal.openFirstAvailable()) {
            return tunerHal;
        }
        return null;
    }

    protected TunerHal(Context context) {
        mIsStreaming = false;
        mFrequency = -1;
        mModulation = null;
    }

    protected boolean isStreaming() {
        return mIsStreaming;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        close();
    }

    protected native void nativeFinalize(long deviceId);

    /**
     * Acquires the first available tuner device. If there is a tuner device that is available, the
     * tuner device will be locked to the current instance.
     *
     * @return {@code true} if the operation was successful, {@code false} otherwise
     */
    protected abstract boolean openFirstAvailable();

    protected abstract boolean isDeviceOpen();

    protected abstract long getDeviceId();

    /**
     * Sets the tuner channel. This should be called after acquiring a tuner device.
     *
     * @param frequency a frequency of the channel to tune to
     * @param modulation a modulation method of the channel to tune to
     * @return {@code true} if the operation was successful, {@code false} otherwise
     */
    public boolean tune(int frequency, @ModulationType String modulation) {
        if (!isDeviceOpen()) {
            Log.e(TAG, "There's no available device");
            return false;
        }
        if (mIsStreaming) {
            nativeCloseAllPidFilters(getDeviceId());
            mIsStreaming = false;
        }

        // When tuning to a new channel in the same frequency, there's no need to stop current tuner
        // device completely and the only thing necessary for tuning is reopening pid filters.
        if (mFrequency == frequency && Objects.equals(mModulation, modulation)) {
            addPidFilter(PID_PAT, FILTER_TYPE_OTHER);
            addPidFilter(PID_ATSC_SI_BASE, FILTER_TYPE_OTHER);
            mIsStreaming = true;
            return true;
        }
        int timeout_ms = modulation.equals(MODULATION_8VSB) ? DEFAULT_VSB_TUNE_TIMEOUT_MS
                : DEFAULT_QAM_TUNE_TIMEOUT_MS;
        if (nativeTune(getDeviceId(), frequency, modulation, timeout_ms)) {
            addPidFilter(PID_PAT, FILTER_TYPE_OTHER);
            addPidFilter(PID_ATSC_SI_BASE, FILTER_TYPE_OTHER);
            mFrequency = frequency;
            mModulation = modulation;
            mIsStreaming = true;
            return true;
        }
        return false;
    }

    protected native boolean nativeTune(long deviceId, int frequency,
            @ModulationType String modulation, int timeout_ms);

    /**
     * Sets a pid filter. This should be set after setting a channel.
     *
     * @param pid a pid number to be added to filter list
     * @param filterType a type of pid. Must be one of (FILTER_TYPE_XXX)
     * @return {@code true} if the operation was successful, {@code false} otherwise
     */
    public boolean addPidFilter(int pid, @FilterType int filterType) {
        if (!isDeviceOpen()) {
            Log.e(TAG, "There's no available device");
            return false;
        }
        if (pid >= 0 && pid <= 0x1fff) {
            nativeAddPidFilter(getDeviceId(), pid, filterType);
            return true;
        }
        return false;
    }

    protected native void nativeAddPidFilter(long deviceId, int pid, @FilterType int filterType);
    protected native void nativeCloseAllPidFilters(long deviceId);

    /**
     * Stops current tuning. The tuner device and pid filters will be reset by this call and make
     * the tuner ready to accept another tune request.
     */
    public void stopTune() {
        if (isDeviceOpen()) {
            if (mIsStreaming) {
                nativeCloseAllPidFilters(getDeviceId());
            }
            nativeStopTune(getDeviceId());
        }
        mIsStreaming = false;
        mFrequency = -1;
        mModulation = null;
    }

    protected native void nativeStopTune(long deviceId);

    /**
     * This method must be called after {@link TunerHal#tune} and before
     * {@link TunerHal#stopStreaming}. Writes at most maxSize TS frames in a buffer
     * provided by the user. The frames employ MPEG encoding.
     *
     * @param javaBuffer a buffer to write the video data in
     * @param javaBufferSize the max amount of bytes to write in this buffer. Usually this number
     *            should be equal to the length of the buffer.
     * @return the amount of bytes written in the buffer. Note that this value could be 0 if no new
     *         frames have been obtained since the last call.
     */
    public int readTsStream(byte[] javaBuffer, int javaBufferSize) {
        if (isDeviceOpen()) {
            return nativeWriteInBuffer(getDeviceId(), javaBuffer, javaBufferSize);
        } else {
            return 0;
        }
    }

    protected native int nativeWriteInBuffer(long deviceId, byte[] javaBuffer, int javaBufferSize);

    /**
     * Opens Linux DVB frontend device. This method is called from native JNI and used only for
     * UsbTunerHal.
     */
    protected int openDvbFrontEndFd() {
        return -1;
    }

    /**
     * Opens Linux DVB demux device. This method is called from native JNI and used only for
     * UsbTunerHal.
     */
    protected int openDvbDemuxFd() {
        return -1;
    }

    /**
     * Opens Linux DVB dvr device. This method is called from native JNI and used only for
     * UsbTunerHal.
     */
    protected int openDvbDvrFd() {
        return -1;
    }
}
