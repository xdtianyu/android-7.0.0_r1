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

package com.android.tv.data.epg;

import android.support.annotation.WorkerThread;

import com.android.tv.data.Channel;
import com.android.tv.data.Program;

import java.util.List;

/**
 * An interface used to retrieve the EPG data. This class should be used in worker thread.
 */
@WorkerThread
public interface EpgReader {
    /**
     * Checks if the reader is available.
     */
    boolean isAvailable();

    /**
     * Returns the timestamp of the current EPG.
     * The format should be YYYYMMDDHHmmSS as a long value. ex) 20160308141500
     */
    long getEpgTimestamp();

    /**
     * Returns the channels list.
     */
    List<Channel> getChannels();

    /**
     * Returns the programs for the given channel. The result is sorted by the start time.
     * Note that the {@code Program} doesn't have valid program ID because it's not retrieved from
     * TvProvider.
     */
    List<Program> getPrograms(long channelId);
}
