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

package com.android.usbtuner.exoplayer.ac3;

import android.os.SystemClock;

/**
 * Copy of {@link com.google.android.exoplayer.MediaClock}.
 * <p>
 * A simple clock for tracking the progression of media time. The clock can be started, stopped and
 * its time can be set and retrieved. When started, this clock is based on
 * {@link SystemClock#elapsedRealtime()}.
 */
/* package */ class AudioClock {
    private boolean mStarted;

    /**
     * The media time when the clock was last set or stopped.
     */
    private long mPositionUs;

    /**
     * The difference between {@link SystemClock#elapsedRealtime()} and {@link #mPositionUs}
     * when the clock was last set or mStarted.
     */
    private long mDeltaUs;

    /**
     * Starts the clock. Does nothing if the clock is already started.
     */
    public void start() {
        if (!mStarted) {
            mStarted = true;
            mDeltaUs = elapsedRealtimeMinus(mPositionUs);
        }
    }

    /**
     * Stops the clock. Does nothing if the clock is already stopped.
     */
    public void stop() {
        if (mStarted) {
            mPositionUs = elapsedRealtimeMinus(mDeltaUs);
            mStarted = false;
        }
    }

    /**
     * @param timeUs The position to set in microseconds.
     */
    public void setPositionUs(long timeUs) {
        this.mPositionUs = timeUs;
        mDeltaUs = elapsedRealtimeMinus(timeUs);
    }

    /**
     * @return The current position in microseconds.
     */
    public long getPositionUs() {
        return mStarted ? elapsedRealtimeMinus(mDeltaUs) : mPositionUs;
    }

    private long elapsedRealtimeMinus(long toSubtractUs) {
        return SystemClock.elapsedRealtime() * 1000 - toSubtractUs;
    }
}
