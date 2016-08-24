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

package com.android.tv;

import static com.android.tv.TimeShiftManager.INVALID_TIME;
import static com.android.tv.TimeShiftManager.REQUEST_TIMEOUT_MS;

import android.test.UiThreadTest;
import android.test.suitebuilder.annotation.MediumTest;

@MediumTest
public class CurrentPositionMediatorTest extends BaseMainActivityTestCase {
    private TimeShiftManager.CurrentPositionMediator mMediator;

    public CurrentPositionMediatorTest() {
        super(MainActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMediator = mActivity.getTimeShiftManager().mCurrentPositionMediator;
    }

    @UiThreadTest
    public void testInitialize() throws Throwable {
        long currentTimeMs = System.currentTimeMillis();
        mMediator.initialize(currentTimeMs);
        assertCurrentPositionMediator(INVALID_TIME, currentTimeMs);
    }

    @UiThreadTest
    public void testOnSeekRequested() throws Throwable {
        long seekToTimeMs = System.currentTimeMillis() - REQUEST_TIMEOUT_MS * 3;
        mMediator.onSeekRequested(seekToTimeMs);
        assertNotSame("Seek request time", INVALID_TIME, mMediator.mSeekRequestTimeMs);
        assertEquals("Current position", seekToTimeMs, mMediator.mCurrentPositionMs);
    }

    @UiThreadTest
    public void testOnCurrentPositionChangedInvalidInput() throws Throwable {
        long seekToTimeMs = System.currentTimeMillis() - REQUEST_TIMEOUT_MS * 3;
        long newCurrentTimeMs = seekToTimeMs + REQUEST_TIMEOUT_MS;
        mMediator.onSeekRequested(seekToTimeMs);
        mMediator.onCurrentPositionChanged(newCurrentTimeMs);
        assertNotSame("Seek request time", INVALID_TIME, mMediator.mSeekRequestTimeMs);
        assertNotSame("Current position", seekToTimeMs, mMediator.mCurrentPositionMs);
        assertNotSame("Current position", newCurrentTimeMs, mMediator.mCurrentPositionMs);
    }

    @UiThreadTest
    public void testOnCurrentPositionChangedValidInput() throws Throwable {
        long seekToTimeMs = System.currentTimeMillis() - REQUEST_TIMEOUT_MS * 3;
        long newCurrentTimeMs = seekToTimeMs + REQUEST_TIMEOUT_MS - 1;
        mMediator.onSeekRequested(seekToTimeMs);
        mMediator.onCurrentPositionChanged(newCurrentTimeMs);
        assertCurrentPositionMediator(INVALID_TIME, newCurrentTimeMs);
    }

    private void assertCurrentPositionMediator(long expectedSeekRequestTimeMs,
            long expectedCurrentPositionMs) {
        assertEquals("Seek request time", expectedSeekRequestTimeMs, mMediator.mSeekRequestTimeMs);
        assertEquals("Current position", expectedCurrentPositionMs, mMediator.mCurrentPositionMs);
    }
}
