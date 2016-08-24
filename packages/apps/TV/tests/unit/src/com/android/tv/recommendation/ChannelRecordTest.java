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

package com.android.tv.recommendation;

import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.tv.testing.Utils;

import java.util.Random;
import java.util.concurrent.TimeUnit;

/**
 * Unit tests for {@link ChannelRecord}.
 */
@SmallTest
public class ChannelRecordTest extends AndroidTestCase {
    private static final int CHANNEL_RECORD_MAX_HISTORY_SIZE = ChannelRecord.MAX_HISTORY_SIZE;

    private Random mRandom;
    private ChannelRecord mChannelRecord;
    private long mLatestWatchEndTimeMs;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mLatestWatchEndTimeMs = System.currentTimeMillis() - TimeUnit.DAYS.toMillis(1);
        mChannelRecord = new ChannelRecord(getContext(), null, false);
        mRandom = Utils.createTestRandom();
    }

    public void testGetLastWatchEndTime_noHistory() {
        assertEquals(0, mChannelRecord.getLastWatchEndTimeMs());
    }

    public void testGetLastWatchEndTime_oneHistory() {
        addWatchLog();

        assertEquals(mLatestWatchEndTimeMs, mChannelRecord.getLastWatchEndTimeMs());
    }

    public void testGetLastWatchEndTime_maxHistories() {
        for (int i = 0; i < CHANNEL_RECORD_MAX_HISTORY_SIZE; ++i) {
            addWatchLog();
        }

        assertEquals(mLatestWatchEndTimeMs, mChannelRecord.getLastWatchEndTimeMs());
    }

    public void testGetLastWatchEndTime_moreThanMaxHistories() {
        for (int i = 0; i < CHANNEL_RECORD_MAX_HISTORY_SIZE + 1; ++i) {
            addWatchLog();
        }

        assertEquals(mLatestWatchEndTimeMs, mChannelRecord.getLastWatchEndTimeMs());
    }

    public void testGetTotalWatchDuration_noHistory() {
        assertEquals(0, mChannelRecord.getTotalWatchDurationMs());
    }

    public void testGetTotalWatchDuration_oneHistory() {
        long durationMs = addWatchLog();

        assertEquals(durationMs, mChannelRecord.getTotalWatchDurationMs());
    }

    public void testGetTotalWatchDuration_maxHistories() {
        long totalWatchTimeMs = 0;
        for (int i = 0; i < CHANNEL_RECORD_MAX_HISTORY_SIZE; ++i) {
            long durationMs = addWatchLog();
            totalWatchTimeMs += durationMs;
        }

        assertEquals(totalWatchTimeMs, mChannelRecord.getTotalWatchDurationMs());
    }

    public void testGetTotalWatchDuration_moreThanMaxHistories() {
        long totalWatchTimeMs = 0;
        long firstDurationMs = 0;
        for (int i = 0; i < CHANNEL_RECORD_MAX_HISTORY_SIZE + 1; ++i) {
            long durationMs = addWatchLog();
            totalWatchTimeMs += durationMs;
            if (i == 0) {
                firstDurationMs = durationMs;
            }
        }

        // Only latest CHANNEL_RECORD_MAX_HISTORY_SIZE logs are remained.
        assertEquals(totalWatchTimeMs - firstDurationMs, mChannelRecord.getTotalWatchDurationMs());
    }

    /**
     * Add new log history to channelRecord which its duration is lower than 1 minute.
     *
     * @return New watch log's duration time in milliseconds.
     */
    private long addWatchLog() {
        // Time hopping with random seconds.
        mLatestWatchEndTimeMs += TimeUnit.SECONDS.toMillis(mRandom.nextInt(60) + 1);

        long durationMs = TimeUnit.SECONDS.toMillis(mRandom.nextInt(60) + 1);
        mChannelRecord.logWatchHistory(new WatchedProgram(null,
                mLatestWatchEndTimeMs, mLatestWatchEndTimeMs + durationMs));
        mLatestWatchEndTimeMs += durationMs;

        return durationMs;
    }
}
