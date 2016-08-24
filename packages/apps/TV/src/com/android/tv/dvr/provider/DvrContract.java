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

package com.android.tv.dvr.provider;

import android.provider.BaseColumns;

/**
 * The contract between the DVR provider and applications. Contains definitions for the supported
 * columns. It's for the internal use in Live TV.
 */
public final class DvrContract {
    /** Column definition for Recording table. */
    public static final class Recordings implements BaseColumns {
        /** The table name. */
        public static final String TABLE_NAME = "recording";

        /** The recording type for program recording. */
        public static final String TYPE_PROGRAM = "TYPE_PROGRAM";

        /** The recording type for timed recording. */
        public static final String TYPE_TIMED = "TYPE_TIMED";

        /** The recording type for season recording. */
        public static final String TYPE_SEASON_RECORDING = "TYPE_SEASON_RECORDING";

        /** The recording has not been started yet. */
        public static final String STATE_RECORDING_NOT_STARTED = "STATE_RECORDING_NOT_STARTED";

        /** The recording is in progress. */
        public static final String STATE_RECORDING_IN_PROGRESS = "STATE_RECORDING_IN_PROGRESS";

        /** The recording was unexpectedly stopped. */
        public static final String STATE_RECORDING_UNEXPECTEDLY_STOPPED =
                "STATE_RECORDING_UNEXPECTEDLY_STOPPED";

        /** The recording is finished. */
        public static final String STATE_RECORDING_FINISHED = "STATE_RECORDING_FINISHED";

        /**
         * The priority of this recording.
         *
         * <p> The lowest number is recorded first. If there is a tie in priority then the lower id
         * wins.  Defaults to {@value Long#MAX_VALUE}
         *
         * <p>Type: INTEGER (long)
         */
        public static final String COLUMN_PRIORITY = "priority";

        /**
         * The type of this recording.
         *
         * <p>This value should be one of the followings: {@link #TYPE_PROGRAM},
         * {@link #TYPE_TIMED}, and {@link #TYPE_SEASON_RECORDING}.
         *
         * <p>This is a required field.
         *
         * <p>Type: String
         */
        public static final String COLUMN_TYPE = "type";

        /**
         * The ID of the channel for recording.
         *
         * <p>This is a required field.
         *
         * <p>Type: INTEGER (long)
         */
        public static final String COLUMN_CHANNEL_ID = "channel_id";


        /**
         * The  ID of the associated program for recording.
         *
         * <p>This is an optional field.
         *
         * <p>Type: INTEGER (long)
         */
        public static final String COLUMN_PROGRAM_ID = "program_id";

        /**
         * The start time of this recording, in milliseconds since the epoch.
         *
         * <p>This is a required field.
         *
         * <p>Type: INTEGER (long)
         */
        public static final String COLUMN_START_TIME_UTC_MILLIS = "start_time_utc_millis";

        /**
         * The end time of this recording, in milliseconds since the epoch.
         *
         * <p>This is a required field.
         *
         * <p>Type: INTEGER (long)
         */
        public static final String COLUMN_END_TIME_UTC_MILLIS = "end_time_utc_millis";

        /**
         * The state of this recording.
         *
         * <p>This value should be one of the followings: {@link #STATE_RECORDING_NOT_STARTED},
         * {@link #STATE_RECORDING_IN_PROGRESS}, {@link #STATE_RECORDING_UNEXPECTEDLY_STOPPED},
         * and {@link #STATE_RECORDING_FINISHED}.
         *
         * <p>This is a required field.
         *
         * <p>Type: String
         */
        public static final String COLUMN_STATE = "state";

        private Recordings() { }
    }

    private DvrContract() { }
}
