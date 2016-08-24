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

package com.android.tv.data;

import android.test.AndroidTestCase;
import android.test.UiThreadTest;
import android.test.suitebuilder.annotation.SmallTest;
import android.test.suitebuilder.annotation.Suppress;

import com.android.tv.data.WatchedHistoryManager.WatchedRecord;
import com.android.tv.testing.Utils;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Test for {@link com.android.tv.data.WatchedHistoryManagerTest}
 */
@SmallTest
@Suppress // http://b/27156462
public class WatchedHistoryManagerTest extends AndroidTestCase {
    private static final boolean DEBUG = false;
    private static final String TAG = "WatchedHistoryManager";

    // Wait time for expected success.
    private static final long WAIT_TIME_OUT_MS = 1000L;
    private static final int MAX_HISTORY_SIZE = 100;

    private WatchedHistoryManager mWatchedHistoryManager;
    private TestWatchedHistoryManagerListener mListener;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        Utils.runOnMainSync(new Runnable() {
            @Override
            public void run() {
                mWatchedHistoryManager = new WatchedHistoryManager(getContext(), MAX_HISTORY_SIZE);
                mListener = new TestWatchedHistoryManagerListener();
                mWatchedHistoryManager.setListener(mListener);
            }
        });
    }

    private void startAndWaitForComplete() throws Exception {
        mWatchedHistoryManager.start();
        try {
            assertTrue(mListener.loadFinishedLatch.await(WAIT_TIME_OUT_MS, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            throw e;
        }
    }

    @UiThreadTest
    public void testIsLoaded() throws Exception {
        assertFalse(mWatchedHistoryManager.isLoaded());
        startAndWaitForComplete();
        assertTrue(mWatchedHistoryManager.isLoaded());
    }

    @UiThreadTest
    public void testLogChannelViewStop() throws Exception {
        startAndWaitForComplete();
        long fakeId = 100000000;
        long time = System.currentTimeMillis();
        long duration = TimeUnit.MINUTES.toMillis(10);
        Channel channel = new Channel.Builder().setId(fakeId).build();
        mWatchedHistoryManager.logChannelViewStop(channel, time, duration);

        WatchedRecord record = mWatchedHistoryManager.getRecord(0);
        WatchedRecord recordFromSharedPreferences =
                mWatchedHistoryManager.getRecordFromSharedPreferences(0);
        assertEquals(record.channelId, fakeId);
        assertEquals(record.watchedStartTime, time - duration);
        assertEquals(record.duration, duration);
        assertEquals(record, recordFromSharedPreferences);
    }

    @UiThreadTest
    public void testCircularHistoryQueue() throws Exception {
        startAndWaitForComplete();
        final long startChannelId = 100000000;
        long time = System.currentTimeMillis();
        long duration = TimeUnit.MINUTES.toMillis(10);

        int size = MAX_HISTORY_SIZE * 2;
        for (int i = 0; i < size; ++i) {
            Channel channel = new Channel.Builder().setId(startChannelId + i).build();
            mWatchedHistoryManager.logChannelViewStop(channel, time + duration * i, duration);
        }
        for (int i = 0; i < MAX_HISTORY_SIZE; ++i) {
            WatchedRecord record = mWatchedHistoryManager.getRecord(i);
            WatchedRecord recordFromSharedPreferences =
                    mWatchedHistoryManager.getRecordFromSharedPreferences(i);
            assertEquals(record, recordFromSharedPreferences);
            assertEquals(record.channelId, startChannelId + size - 1 - i);
        }
        // Since the WatchedHistory is a circular queue, the value for 0 and maxHistorySize
        // are same.
        assertEquals(mWatchedHistoryManager.getRecordFromSharedPreferences(0),
                mWatchedHistoryManager.getRecordFromSharedPreferences(MAX_HISTORY_SIZE));
    }

    @UiThreadTest
    public void testWatchedRecordEquals() {
        assertTrue(new WatchedRecord(1, 2, 3).equals(new WatchedRecord(1, 2, 3)));
        assertFalse(new WatchedRecord(1, 2, 3).equals(new WatchedRecord(1, 2, 4)));
        assertFalse(new WatchedRecord(1, 2, 3).equals(new WatchedRecord(1, 4, 3)));
        assertFalse(new WatchedRecord(1, 2, 3).equals(new WatchedRecord(4, 2, 3)));
    }

    @UiThreadTest
    public void testEncodeDecodeWatchedRecord() throws Exception {
        long fakeId = 100000000;
        long time = System.currentTimeMillis();
        long duration = TimeUnit.MINUTES.toMillis(10);
        WatchedRecord record = new WatchedRecord(fakeId, time, duration);
        WatchedRecord sameRecord = mWatchedHistoryManager.decode(
                mWatchedHistoryManager.encode(record));
        assertEquals(record, sameRecord);
    }

    private class TestWatchedHistoryManagerListener implements WatchedHistoryManager.Listener {
        public CountDownLatch loadFinishedLatch = new CountDownLatch(1);

        @Override
        public void onLoadFinished() {
            loadFinishedLatch.countDown();
        }

        @Override
        public void onNewRecordAdded(WatchedRecord watchedRecord) { }
    }
}
