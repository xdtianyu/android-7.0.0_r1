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
 * limitations under the License
 */

package com.android.tv.testing.dvr;

import com.android.tv.dvr.DvrDataManagerInMemoryImpl;
import com.android.tv.dvr.ScheduledRecording;

import junit.framework.Assert;

/**
 * Static utils for using {@link ScheduledRecording} in tests.
 */
public final class RecordingTestUtils {
    public static ScheduledRecording createTestRecordingWithIdAndPeriod(long id, long channelId,
            long startTime, long endTime) {
        return ScheduledRecording.builder(startTime, endTime)
                .setId(id)
                .setChannelId(channelId)
                .build();
    }

    public static ScheduledRecording createTestRecordingWithPeriod(long channelId, long startTime,
            long endTime) {
        return createTestRecordingWithIdAndPeriod(ScheduledRecording.ID_NOT_SET, channelId,
                startTime, endTime);
    }

    public static ScheduledRecording addScheduledRecording(
            DvrDataManagerInMemoryImpl dvrDataManager, long channelId, long startTime,
            long endTime) {
        ScheduledRecording recording = createTestRecordingWithPeriod(channelId, startTime, endTime);
        recording = dvrDataManager.addScheduledRecordingInternal(recording);
        return recording;
    }

    public static ScheduledRecording normalizePriority(ScheduledRecording orig){
        return ScheduledRecording.buildFrom(orig).setPriority(orig.getId()).build();
    }

    public static void assertRecordingEquals(ScheduledRecording expected, ScheduledRecording actual) {
        Assert.assertEquals("id", expected.getId(), actual.getId());
        Assert.assertEquals("channel", expected.getChannelId(), actual.getChannelId());
        Assert.assertEquals("programId", expected.getProgramId(), actual.getProgramId());
        Assert.assertEquals("priority", expected.getPriority(), actual.getPriority());
        Assert.assertEquals("start time", expected.getStartTimeMs(), actual.getStartTimeMs());
        Assert.assertEquals("end time", expected.getEndTimeMs(), actual.getEndTimeMs());
        Assert.assertEquals("state", expected.getState(), actual.getState());
        Assert.assertEquals("parent season recording", expected.getParentSeasonRecording(),
                actual.getParentSeasonRecording());
    }

    private RecordingTestUtils() { }
}
