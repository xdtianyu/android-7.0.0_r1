/*
 * Copyright (C) 2016 Google Inc.
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

package android.location.cts;

import android.location.GnssStatus;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Used for receiving notifications when GNSS status has changed.
 */
class TestGnssStatusCallback extends GnssStatus.Callback {

    private volatile boolean mGpsStatusReceived;
    private GnssStatus mGnssStatus = null;
    // Timeout in sec for count down latch wait
    private static final int TIMEOUT_IN_SEC = 90;
    private final CountDownLatch mCountDownLatch;
    // Store list of Prn for Satellites.
    private List<List<Integer>> mGpsSatellitePrns;

    TestGnssStatusCallback(int gpsStatusCountToCollect) {
        mCountDownLatch = new CountDownLatch(gpsStatusCountToCollect);
        mGpsSatellitePrns = new ArrayList<List<Integer>>();
    }

    @Override
    public void onStarted() {
    }

    @Override
    public void onStopped() {
    }

    @Override
    public void onFirstFix(int ttffMillis) {
    }

    @Override
    public void onSatelliteStatusChanged(GnssStatus status) {
        mCountDownLatch.countDown();
    }

    /**
     * Returns the list of PRNs (pseudo-random number) for the satellite.
     *
     * @return list of PRNs number
     */
    public List<List<Integer>> getGpsSatellitePrns() {
        return mGpsSatellitePrns;
    }

    /**
     * Check if GPS Status is received.
     *
     * @return {@code true} if the GPS Status is received and {@code false}
     *         if GPS Status is not received.
     */
    public boolean isGpsStatusReceived() {
        return mGpsStatusReceived;
    }

    /**
     * Get GPS Status.
     *
     * @return mGpsStatus GPS Status
     */
    public GnssStatus getGnssStatus() {
        return mGnssStatus;
    }

    public boolean await() throws InterruptedException {
        return TestUtils.waitFor(mCountDownLatch);
    }
}
