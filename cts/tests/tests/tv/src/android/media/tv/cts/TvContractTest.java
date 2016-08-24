/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.media.tv.cts;

import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.res.AssetFileDescriptor;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.tv.TvContentRating;
import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import android.media.tv.TvContract.Programs.Genres;
import android.media.tv.TvContract.RecordedPrograms;
import android.net.Uri;
import android.test.AndroidTestCase;
import android.test.MoreAsserts;
import android.tv.cts.R;

import java.io.InputStream;
import java.io.OutputStream;
import java.util.Arrays;
import java.util.List;

/**
 * Test for {@link android.media.tv.TvContract}.
 */
public class TvContractTest extends AndroidTestCase {
    private static final String[] CHANNELS_PROJECTION = {
        TvContract.Channels._ID,
        TvContract.Channels.COLUMN_INPUT_ID,
        TvContract.Channels.COLUMN_TYPE,
        TvContract.Channels.COLUMN_SERVICE_TYPE,
        TvContract.Channels.COLUMN_ORIGINAL_NETWORK_ID,
        TvContract.Channels.COLUMN_TRANSPORT_STREAM_ID,
        TvContract.Channels.COLUMN_SERVICE_ID,
        TvContract.Channels.COLUMN_DISPLAY_NUMBER,
        TvContract.Channels.COLUMN_DISPLAY_NAME,
        TvContract.Channels.COLUMN_NETWORK_AFFILIATION,
        TvContract.Channels.COLUMN_DESCRIPTION,
        TvContract.Channels.COLUMN_VIDEO_FORMAT,
        TvContract.Channels.COLUMN_INTERNAL_PROVIDER_DATA,
        TvContract.Channels.COLUMN_VERSION_NUMBER,
    };

    private static final String[] PROGRAMS_PROJECTION = {
        TvContract.Programs._ID,
        TvContract.Programs.COLUMN_CHANNEL_ID,
        TvContract.Programs.COLUMN_TITLE,
        TvContract.Programs.COLUMN_SEASON_DISPLAY_NUMBER,
        TvContract.Programs.COLUMN_SEASON_TITLE,
        TvContract.Programs.COLUMN_EPISODE_DISPLAY_NUMBER,
        TvContract.Programs.COLUMN_EPISODE_TITLE,
        TvContract.Programs.COLUMN_START_TIME_UTC_MILLIS,
        TvContract.Programs.COLUMN_END_TIME_UTC_MILLIS,
        TvContract.Programs.COLUMN_BROADCAST_GENRE,
        TvContract.Programs.COLUMN_CANONICAL_GENRE,
        TvContract.Programs.COLUMN_SHORT_DESCRIPTION,
        TvContract.Programs.COLUMN_LONG_DESCRIPTION,
        TvContract.Programs.COLUMN_VIDEO_WIDTH,
        TvContract.Programs.COLUMN_VIDEO_HEIGHT,
        TvContract.Programs.COLUMN_AUDIO_LANGUAGE,
        TvContract.Programs.COLUMN_CONTENT_RATING,
        TvContract.Programs.COLUMN_POSTER_ART_URI,
        TvContract.Programs.COLUMN_THUMBNAIL_URI,
        TvContract.Programs.COLUMN_INTERNAL_PROVIDER_DATA,
        TvContract.Programs.COLUMN_VERSION_NUMBER,
    };

    private static long OPERATION_TIME = 1000l;

    private static final String ENCODED_GENRE_STRING = Genres.ANIMAL_WILDLIFE + "," + Genres.COMEDY
            + "," + Genres.DRAMA + "," + Genres.EDUCATION + "," + Genres.FAMILY_KIDS + ","
            + Genres.GAMING + "," + Genres.MOVIES + "," + Genres.NEWS + "," + Genres.SHOPPING + ","
            + Genres.SPORTS + "," + Genres.TRAVEL;

    // Delimiter for genre.
    private static final String DELIMITER = ",";
    private static final String EMPTY_GENRE = "";
    private static final String COMMA = ",";
    private static final String COMMA_ENCODED = "\",";
    private static final String QUOTE = "\"";
    private static final String QUOTE_ENCODED = "\"\"";
    private static final String WHITE_SPACES = " \r \n \t \f ";

    private static final String PARAM_CANONICAL_GENRE = "canonical_genre";

    private String mInputId;
    private ContentResolver mContentResolver;
    private Uri mChannelsUri;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        mInputId = TvContract.buildInputId(
                new ComponentName(getContext(), StubTunerTvInputService.class));
        mContentResolver = getContext().getContentResolver();
        mChannelsUri = TvContract.buildChannelsUriForInput(mInputId);
    }

    @Override
    protected void tearDown() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            super.tearDown();
            return;
        }
        // Clean up, just in case we failed to delete the entry when a test failed.
        // The cotentUris are specific to this package, so this will delete only the
        // entries inserted by this package.
        String[] projection = { TvContract.Channels._ID };
        try (Cursor cursor = mContentResolver.query(mChannelsUri, projection, null, null, null)) {
            while (cursor != null && cursor.moveToNext()) {
                long channelId = cursor.getLong(0);
                mContentResolver.delete(
                        TvContract.buildProgramsUriForChannel(channelId), null, null);
            }
        }
        mContentResolver.delete(mChannelsUri, null, null);
        mContentResolver.delete(RecordedPrograms.CONTENT_URI, null, null);
        super.tearDown();
    }

    private static ContentValues createDummyChannelValues(String inputId) {
        ContentValues values = new ContentValues();
        values.put(TvContract.Channels.COLUMN_INPUT_ID, inputId);
        values.put(TvContract.Channels.COLUMN_TYPE, TvContract.Channels.TYPE_OTHER);
        values.put(TvContract.Channels.COLUMN_SERVICE_TYPE,
                TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO);
        values.put(TvContract.Channels.COLUMN_DISPLAY_NUMBER, "1");
        values.put(TvContract.Channels.COLUMN_VIDEO_FORMAT, TvContract.Channels.VIDEO_FORMAT_480P);

        return values;
    }

    private static ContentValues createDummyProgramValues(long channelId) {
        ContentValues values = new ContentValues();
        values.put(TvContract.Programs.COLUMN_CHANNEL_ID, channelId);
        values.put(TvContract.Programs.COLUMN_EPISODE_DISPLAY_NUMBER , "1A");
        values.put(TvContract.Programs.COLUMN_EPISODE_TITLE, "episode_title");
        values.put(TvContract.Programs.COLUMN_SEASON_DISPLAY_NUMBER , "2B");
        values.put(TvContract.Programs.COLUMN_SEASON_TITLE, "season_title");
        values.put(TvContract.Programs.COLUMN_CANONICAL_GENRE, TvContract.Programs.Genres.encode(
                TvContract.Programs.Genres.MOVIES, TvContract.Programs.Genres.DRAMA));
        TvContentRating rating = TvContentRating.createRating("android.media.tv", "US_TVPG",
                "US_TVPG_TV_MA", "US_TVPG_S", "US_TVPG_V");
        values.put(TvContract.Programs.COLUMN_CONTENT_RATING, rating.flattenToString());

        return values;
    }

    private static ContentValues createDummyRecordedProgramValues(String inputId, long channelId) {
        ContentValues values = new ContentValues();
        values.put(TvContract.RecordedPrograms.COLUMN_INPUT_ID, inputId);
        values.put(TvContract.RecordedPrograms.COLUMN_CHANNEL_ID, channelId);
        values.put(TvContract.RecordedPrograms.COLUMN_SEASON_DISPLAY_NUMBER , "3B");
        values.put(TvContract.RecordedPrograms.COLUMN_SEASON_TITLE, "season_title");
        values.put(TvContract.RecordedPrograms.COLUMN_EPISODE_DISPLAY_NUMBER , "2A");
        values.put(TvContract.RecordedPrograms.COLUMN_EPISODE_TITLE, "episode_title");
        values.put(TvContract.RecordedPrograms.COLUMN_START_TIME_UTC_MILLIS, 1000);
        values.put(TvContract.RecordedPrograms.COLUMN_END_TIME_UTC_MILLIS, 2000);
        values.put(TvContract.RecordedPrograms.COLUMN_CANONICAL_GENRE,
                TvContract.Programs.Genres.encode(
                        TvContract.Programs.Genres.MOVIES, TvContract.Programs.Genres.DRAMA));
        values.put(TvContract.RecordedPrograms.COLUMN_SHORT_DESCRIPTION, "short_description");
        values.put(TvContract.RecordedPrograms.COLUMN_LONG_DESCRIPTION, "long_description");
        values.put(TvContract.RecordedPrograms.COLUMN_VIDEO_WIDTH, 1920);
        values.put(TvContract.RecordedPrograms.COLUMN_VIDEO_HEIGHT, 1080);
        values.put(TvContract.RecordedPrograms.COLUMN_AUDIO_LANGUAGE, "en");
        TvContentRating rating = TvContentRating.createRating("android.media.tv", "US_TVPG",
                "US_TVPG_TV_MA", "US_TVPG_S", "US_TVPG_V");
        values.put(TvContract.RecordedPrograms.COLUMN_CONTENT_RATING, rating.flattenToString());
        values.put(TvContract.RecordedPrograms.COLUMN_POSTER_ART_URI, "http://foo.com/artwork.png");
        values.put(TvContract.RecordedPrograms.COLUMN_THUMBNAIL_URI, "http://foo.com/thumbnail.jpg");
        values.put(TvContract.RecordedPrograms.COLUMN_SEARCHABLE, 1);
        values.put(TvContract.RecordedPrograms.COLUMN_RECORDING_DATA_URI, "file:///sdcard/foo/");
        values.put(TvContract.RecordedPrograms.COLUMN_RECORDING_DATA_BYTES, 1024 * 1024);
        values.put(TvContract.RecordedPrograms.COLUMN_RECORDING_DURATION_MILLIS, 60 * 60 * 1000);
        values.put(TvContract.RecordedPrograms.COLUMN_RECORDING_EXPIRE_TIME_UTC_MILLIS, 1454480880L);
        values.put(TvContract.RecordedPrograms.COLUMN_INTERNAL_PROVIDER_DATA,
                "internal_provider_data".getBytes());
        values.put(TvContract.RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG1, 4);
        values.put(TvContract.RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG2, 3);
        values.put(TvContract.RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG3, 2);
        values.put(TvContract.RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG4, 1);

        return values;
    }

    private static void verifyStringColumn(Cursor cursor, ContentValues expectedValues,
            String columnName) {
        if (expectedValues.containsKey(columnName)) {
            assertEquals(expectedValues.getAsString(columnName),
                    cursor.getString(cursor.getColumnIndex(columnName)));
        }
    }

    private static void verifyIntegerColumn(Cursor cursor, ContentValues expectedValues,
            String columnName) {
        if (expectedValues.containsKey(columnName)) {
            assertEquals(expectedValues.getAsInteger(columnName).intValue(),
                    cursor.getInt(cursor.getColumnIndex(columnName)));
        }
    }

    private static void verifyLongColumn(Cursor cursor, ContentValues expectedValues,
            String columnName) {
        if (expectedValues.containsKey(columnName)) {
            assertEquals(expectedValues.getAsLong(columnName).longValue(),
                    cursor.getLong(cursor.getColumnIndex(columnName)));
        }
    }

    private static void verifyBlobColumn(Cursor cursor, ContentValues expectedValues,
            String columnName) {
        if (expectedValues.containsKey(columnName)) {
            byte[] expected = expectedValues.getAsByteArray(columnName);
            byte[] actual = cursor.getBlob(cursor.getColumnIndex(columnName));
            assertEquals(expected.length, actual.length);
            for (int i = 0; i < expected.length; ++i) {
                assertEquals(expected[i], actual[i]);
            }
        }
    }

    private void verifyChannel(Uri channelUri, ContentValues expectedValues, long channelId) {
        try (Cursor cursor = mContentResolver.query(
                channelUri, CHANNELS_PROJECTION, null, null, null)) {
            assertNotNull(cursor);
            assertEquals(cursor.getCount(), 1);
            assertTrue(cursor.moveToNext());
            assertEquals(channelId, cursor.getLong(cursor.getColumnIndex(TvContract.Channels._ID)));
            verifyStringColumn(cursor, expectedValues, TvContract.Channels.COLUMN_INPUT_ID);
            verifyStringColumn(cursor, expectedValues, TvContract.Channels.COLUMN_TYPE);
            verifyStringColumn(cursor, expectedValues, TvContract.Channels.COLUMN_SERVICE_TYPE);
            verifyIntegerColumn(cursor, expectedValues,
                    TvContract.Channels.COLUMN_ORIGINAL_NETWORK_ID);
            verifyIntegerColumn(cursor, expectedValues,
                    TvContract.Channels.COLUMN_TRANSPORT_STREAM_ID);
            verifyIntegerColumn(cursor, expectedValues,
                    TvContract.Channels.COLUMN_SERVICE_ID);
            verifyStringColumn(cursor, expectedValues, TvContract.Channels.COLUMN_DISPLAY_NUMBER);
            verifyStringColumn(cursor, expectedValues, TvContract.Channels.COLUMN_DISPLAY_NAME);
            verifyStringColumn(cursor, expectedValues,
                    TvContract.Channels.COLUMN_NETWORK_AFFILIATION);
            verifyStringColumn(cursor, expectedValues, TvContract.Channels.COLUMN_DESCRIPTION);
            verifyStringColumn(cursor, expectedValues, TvContract.Channels.COLUMN_VIDEO_FORMAT);
            verifyBlobColumn(cursor, expectedValues,
                    TvContract.Channels.COLUMN_INTERNAL_PROVIDER_DATA);
            verifyIntegerColumn(cursor, expectedValues, TvContract.Channels.COLUMN_VERSION_NUMBER);
        }
    }

    public void testChannelsTable() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        // Test: insert
        ContentValues values = createDummyChannelValues(mInputId);

        Uri rowUri = mContentResolver.insert(mChannelsUri, values);
        long channelId = ContentUris.parseId(rowUri);
        Uri channelUri = TvContract.buildChannelUri(channelId);
        verifyChannel(channelUri, values, channelId);

        // Test: update
        values.put(TvContract.Channels.COLUMN_DISPLAY_NUMBER, "1-1");
        values.put(TvContract.Channels.COLUMN_DISPLAY_NAME, "One dash one");
        values.put(TvContract.Channels.COLUMN_INTERNAL_PROVIDER_DATA, "Coffee".getBytes());

        mContentResolver.update(channelUri, values, null, null);
        verifyChannel(channelUri, values, channelId);

        // Test: delete
        mContentResolver.delete(mChannelsUri, null, null);
        try (Cursor cursor = mContentResolver.query(
                mChannelsUri, CHANNELS_PROJECTION, null, null, null)) {
            assertEquals(0, cursor.getCount());
        }
    }

    private void verifyProgram(Uri programUri, ContentValues expectedValues, long programId) {
        try (Cursor cursor = mContentResolver.query(
                programUri, PROGRAMS_PROJECTION, null, null, null)) {
            assertNotNull(cursor);
            assertEquals(cursor.getCount(), 1);
            assertTrue(cursor.moveToNext());
            assertEquals(programId, cursor.getLong(cursor.getColumnIndex(TvContract.Programs._ID)));
            verifyLongColumn(cursor, expectedValues, TvContract.Programs.COLUMN_CHANNEL_ID);
            verifyStringColumn(cursor, expectedValues, TvContract.Programs.COLUMN_TITLE);
            verifyStringColumn(cursor, expectedValues,
                    TvContract.Programs.COLUMN_SEASON_DISPLAY_NUMBER);
            verifyStringColumn(cursor, expectedValues, TvContract.Programs.COLUMN_SEASON_TITLE);
            verifyStringColumn(cursor, expectedValues,
                    TvContract.Programs.COLUMN_EPISODE_DISPLAY_NUMBER);
            verifyStringColumn(cursor, expectedValues, TvContract.Programs.COLUMN_EPISODE_TITLE);
            verifyLongColumn(cursor, expectedValues,
                    TvContract.Programs.COLUMN_START_TIME_UTC_MILLIS);
            verifyLongColumn(cursor, expectedValues,
                    TvContract.Programs.COLUMN_END_TIME_UTC_MILLIS);
            verifyStringColumn(cursor, expectedValues, TvContract.Programs.COLUMN_BROADCAST_GENRE);
            verifyStringColumn(cursor, expectedValues, TvContract.Programs.COLUMN_CANONICAL_GENRE);
            verifyStringColumn(cursor, expectedValues,
                    TvContract.Programs.COLUMN_SHORT_DESCRIPTION);
            verifyStringColumn(cursor, expectedValues, TvContract.Programs.COLUMN_LONG_DESCRIPTION);
            verifyIntegerColumn(cursor, expectedValues, TvContract.Programs.COLUMN_VIDEO_WIDTH);
            verifyIntegerColumn(cursor, expectedValues, TvContract.Programs.COLUMN_VIDEO_HEIGHT);
            verifyStringColumn(cursor, expectedValues, TvContract.Programs.COLUMN_AUDIO_LANGUAGE);
            verifyStringColumn(cursor, expectedValues, TvContract.Programs.COLUMN_CONTENT_RATING);
            verifyStringColumn(cursor, expectedValues, TvContract.Programs.COLUMN_POSTER_ART_URI);
            verifyStringColumn(cursor, expectedValues, TvContract.Programs.COLUMN_THUMBNAIL_URI);
            verifyBlobColumn(cursor, expectedValues,
                    TvContract.Programs.COLUMN_INTERNAL_PROVIDER_DATA);
            verifyIntegerColumn(cursor, expectedValues, TvContract.Programs.COLUMN_VERSION_NUMBER);
        }
    }

    private void verifyDeprecatedColumsInProgram(Uri programUri, ContentValues expectedValues) {
        final String[] DEPRECATED_COLUMNS_PROJECTION = {
            TvContract.Programs.COLUMN_SEASON_NUMBER,
            TvContract.Programs.COLUMN_EPISODE_NUMBER,
        };
        try (Cursor cursor = mContentResolver.query(
                programUri, DEPRECATED_COLUMNS_PROJECTION, null, null, null)) {
            assertNotNull(cursor);
            assertEquals(cursor.getCount(), 1);
            assertTrue(cursor.moveToNext());
            verifyIntegerColumn(cursor, expectedValues, TvContract.Programs.COLUMN_SEASON_NUMBER);
            verifyIntegerColumn(cursor, expectedValues, TvContract.Programs.COLUMN_EPISODE_NUMBER);
        }
    }

    private void verifyLogoIsReadable(Uri logoUri) throws Exception {
        try (AssetFileDescriptor fd = mContentResolver.openAssetFileDescriptor(logoUri, "r")) {
            try (InputStream is = fd.createInputStream()) {
                // Assure that the stream is decodable as a Bitmap.
                BitmapFactory.decodeStream(is);
            }
        }
    }

    public void testChannelLogo() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        // Set-up: add a channel.
        ContentValues values = createDummyChannelValues(mInputId);
        Uri channelUri = mContentResolver.insert(mChannelsUri, values);
        Uri logoUri = TvContract.buildChannelLogoUri(channelUri);
        Bitmap logo = BitmapFactory.decodeResource(getContext().getResources(), R.drawable.robot);

        // Write
        try (AssetFileDescriptor fd = mContentResolver.openAssetFileDescriptor(logoUri, "w")) {
            try (OutputStream os = fd.createOutputStream()) {
                logo.compress(Bitmap.CompressFormat.PNG, 100, os);
            }
        }

        // Give some time for TvProvider to process the logo.
        Thread.sleep(OPERATION_TIME);

        // Read and verify
        verifyLogoIsReadable(logoUri);

        // Read and verify using alternative logo URI.
        verifyLogoIsReadable(TvContract.buildChannelLogoUri(ContentUris.parseId(channelUri)));
    }

    public void verifyProgramsTable(Uri programsUri, long channelId) {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        // Test: insert
        ContentValues values = createDummyProgramValues(channelId);

        Uri rowUri = mContentResolver.insert(programsUri, values);
        long programId = ContentUris.parseId(rowUri);
        Uri programUri = TvContract.buildProgramUri(programId);
        verifyProgram(programUri, values, programId);

        // Test: update
        values.put(TvContract.Programs.COLUMN_EPISODE_TITLE, "Sample title");
        values.put(TvContract.Programs.COLUMN_SHORT_DESCRIPTION, "Short description");
        values.put(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_DATA, "Coffee".getBytes());

        mContentResolver.update(programUri, values, null, null);
        verifyProgram(programUri, values, programId);

        // Test: delete
        mContentResolver.delete(programsUri, null, null);
        try (Cursor cursor = mContentResolver.query(
                programsUri, PROGRAMS_PROJECTION, null, null, null)) {
            assertEquals(0, cursor.getCount());
        }
    }

    public void verifyProgramsTableWithDeprecatedColumns(Uri programsUri, long channelId) {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        // Test: insert
        ContentValues expected = createDummyProgramValues(channelId);
        expected.put(TvContract.Programs.COLUMN_SEASON_DISPLAY_NUMBER, "3");
        expected.put(TvContract.Programs.COLUMN_EPISODE_DISPLAY_NUMBER, "9");

        ContentValues input = new ContentValues(expected);
        input.remove(TvContract.Programs.COLUMN_SEASON_DISPLAY_NUMBER);
        input.remove(TvContract.Programs.COLUMN_EPISODE_DISPLAY_NUMBER);
        input.put(TvContract.Programs.COLUMN_SEASON_NUMBER, 3);
        input.put(TvContract.Programs.COLUMN_EPISODE_NUMBER, 9);

        Uri rowUri = mContentResolver.insert(programsUri, input);
        long programId = ContentUris.parseId(rowUri);
        Uri programUri = TvContract.buildProgramUri(programId);
        verifyProgram(programUri, expected, programId);
        verifyDeprecatedColumsInProgram(programUri, input);

        // Test: update
        expected.put(TvContract.Programs.COLUMN_SEASON_DISPLAY_NUMBER, "4");
        expected.put(TvContract.Programs.COLUMN_EPISODE_DISPLAY_NUMBER, "10");
        input.put(TvContract.Programs.COLUMN_SEASON_NUMBER, 4);
        input.put(TvContract.Programs.COLUMN_EPISODE_NUMBER, 10);

        mContentResolver.update(programUri, input, null, null);
        verifyProgram(programUri, expected, programId);
        verifyDeprecatedColumsInProgram(programUri, input);

        // Test: delete
        mContentResolver.delete(programsUri, null, null);
        try (Cursor cursor = mContentResolver.query(
                programsUri, PROGRAMS_PROJECTION, null, null, null)) {
            assertEquals(0, cursor.getCount());
        }
    }

    public void testProgramsTable() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        // Set-up: add a channel.
        ContentValues values = createDummyChannelValues(mInputId);
        Uri channelUri = mContentResolver.insert(mChannelsUri, values);
        long channelId = ContentUris.parseId(channelUri);

        verifyProgramsTable(TvContract.buildProgramsUriForChannel(channelId), channelId);
        verifyProgramsTable(TvContract.buildProgramsUriForChannel(channelUri), channelId);
        verifyProgramsTableWithDeprecatedColumns(TvContract.buildProgramsUriForChannel(channelId),
                channelId);
        verifyProgramsTableWithDeprecatedColumns(TvContract.buildProgramsUriForChannel(channelUri),
                channelId);
    }

    private void verifyOverlap(long startMillis, long endMillis, int expectedCount,
            long channelId, Uri channelUri) {
        try (Cursor cursor = mContentResolver.query(TvContract.buildProgramsUriForChannel(
                channelId, startMillis, endMillis), PROGRAMS_PROJECTION, null, null, null)) {
            assertEquals(expectedCount, cursor.getCount());
        }
        try (Cursor cursor = mContentResolver.query(TvContract.buildProgramsUriForChannel(
                channelUri, startMillis, endMillis), PROGRAMS_PROJECTION, null, null, null)) {
            assertEquals(expectedCount, cursor.getCount());
        }
    }

    public void testProgramsScheduleOverlap() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        final long programStartMillis = 1403712000000l;  // Jun 25 2014 16:00 UTC
        final long programEndMillis = 1403719200000l;  // Jun 25 2014 18:00 UTC
        final long hour = 3600000l;

        // Set-up: add a channel and program.
        ContentValues values = createDummyChannelValues(mInputId);
        Uri channelUri = mContentResolver.insert(mChannelsUri, values);
        long channelId = ContentUris.parseId(channelUri);
        Uri programsUri = TvContract.buildProgramsUriForChannel(channelId);
        values = createDummyProgramValues(channelId);
        values.put(TvContract.Programs.COLUMN_START_TIME_UTC_MILLIS, programStartMillis);
        values.put(TvContract.Programs.COLUMN_END_TIME_UTC_MILLIS, programEndMillis);
        mContentResolver.insert(programsUri, values);

        // Overlap 1: starts early, ends early.
        verifyOverlap(programStartMillis - hour, programEndMillis - hour, 1, channelId, channelUri);

        // Overlap 2: starts early, ends late.
        verifyOverlap(programStartMillis - hour, programEndMillis + hour, 1, channelId, channelUri);

        // Overlap 3: starts early, ends late.
        verifyOverlap(programStartMillis + hour / 2, programEndMillis - hour / 2, 1,
                channelId, channelUri);

        // Overlap 4: starts late, ends late.
        verifyOverlap(programStartMillis + hour, programEndMillis + hour, 1, channelId, channelUri);

        // Non-overlap 1: ends too early.
        verifyOverlap(programStartMillis - hour, programStartMillis - hour / 2, 0,
                channelId, channelUri);

        // Non-overlap 2: starts too late
        verifyOverlap(programEndMillis + hour, programEndMillis + hour * 2, 0,
                channelId, channelUri);
    }

    private void verifyRecordedProgram(Uri recordedProgramUri, ContentValues expectedValues,
                long recordedProgramId) {
        try (Cursor cursor = mContentResolver.query(recordedProgramUri, null, null, null, null)) {
            assertNotNull(cursor);
            assertEquals(cursor.getCount(), 1);
            assertTrue(cursor.moveToNext());
            assertEquals(recordedProgramId, cursor.getLong(cursor.getColumnIndex(
                    RecordedPrograms._ID)));
            verifyLongColumn(cursor, expectedValues, RecordedPrograms.COLUMN_CHANNEL_ID);
            verifyStringColumn(cursor, expectedValues,
                    RecordedPrograms.COLUMN_SEASON_DISPLAY_NUMBER);
            verifyStringColumn(cursor, expectedValues, RecordedPrograms.COLUMN_SEASON_TITLE);
            verifyStringColumn(cursor, expectedValues,
                    RecordedPrograms.COLUMN_EPISODE_DISPLAY_NUMBER);
            verifyStringColumn(cursor, expectedValues, RecordedPrograms.COLUMN_EPISODE_TITLE);
            verifyLongColumn(cursor, expectedValues, RecordedPrograms.COLUMN_START_TIME_UTC_MILLIS);
            verifyLongColumn(cursor, expectedValues, RecordedPrograms.COLUMN_END_TIME_UTC_MILLIS);
            verifyStringColumn(cursor, expectedValues, RecordedPrograms.COLUMN_BROADCAST_GENRE);
            verifyStringColumn(cursor, expectedValues, RecordedPrograms.COLUMN_CANONICAL_GENRE);
            verifyStringColumn(cursor, expectedValues, RecordedPrograms.COLUMN_SHORT_DESCRIPTION);
            verifyStringColumn(cursor, expectedValues, RecordedPrograms.COLUMN_LONG_DESCRIPTION);
            verifyIntegerColumn(cursor, expectedValues, RecordedPrograms.COLUMN_VIDEO_WIDTH);
            verifyIntegerColumn(cursor, expectedValues, RecordedPrograms.COLUMN_VIDEO_HEIGHT);
            verifyStringColumn(cursor, expectedValues, RecordedPrograms.COLUMN_AUDIO_LANGUAGE);
            verifyStringColumn(cursor, expectedValues, RecordedPrograms.COLUMN_CONTENT_RATING);
            verifyStringColumn(cursor, expectedValues, RecordedPrograms.COLUMN_POSTER_ART_URI);
            verifyStringColumn(cursor, expectedValues, RecordedPrograms.COLUMN_THUMBNAIL_URI);
            verifyIntegerColumn(cursor, expectedValues, RecordedPrograms.COLUMN_SEARCHABLE);
            verifyStringColumn(cursor, expectedValues, RecordedPrograms.COLUMN_RECORDING_DATA_URI);
            verifyIntegerColumn(cursor, expectedValues,
                    RecordedPrograms.COLUMN_RECORDING_DATA_BYTES);
            verifyIntegerColumn(cursor, expectedValues,
                    RecordedPrograms.COLUMN_RECORDING_DURATION_MILLIS);
            verifyIntegerColumn(cursor, expectedValues,
                    RecordedPrograms.COLUMN_RECORDING_EXPIRE_TIME_UTC_MILLIS);
            verifyBlobColumn(cursor, expectedValues,
                    RecordedPrograms.COLUMN_INTERNAL_PROVIDER_DATA);
            verifyIntegerColumn(cursor, expectedValues,
                    RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG1);
            verifyIntegerColumn(cursor, expectedValues,
                    RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG2);
            verifyIntegerColumn(cursor, expectedValues,
                    RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG3);
            verifyIntegerColumn(cursor, expectedValues,
                    RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG4);
            verifyIntegerColumn(cursor, expectedValues, RecordedPrograms.COLUMN_VERSION_NUMBER);
        }
    }

    private void verifyRecordedProgramsTable(Uri recordedProgramsUri, long channelId) {
        // Test: insert
        ContentValues values = createDummyRecordedProgramValues(mInputId, channelId);

        Uri rowUri = mContentResolver.insert(recordedProgramsUri, values);
        long recordedProgramId = ContentUris.parseId(rowUri);
        Uri recordedProgramUri = TvContract.buildRecordedProgramUri(recordedProgramId);
        verifyRecordedProgram(recordedProgramUri, values, recordedProgramId);

        // Test: update
        values.put(TvContract.RecordedPrograms.COLUMN_EPISODE_TITLE, "episode_title1");
        values.put(TvContract.RecordedPrograms.COLUMN_SHORT_DESCRIPTION, "short_description1");
        values.put(TvContract.RecordedPrograms.COLUMN_INTERNAL_PROVIDER_DATA,
                "internal_provider_data1".getBytes());

        mContentResolver.update(recordedProgramUri, values, null, null);
        verifyRecordedProgram(recordedProgramUri, values, recordedProgramId);

        // Test: delete
        mContentResolver.delete(recordedProgramUri, null, null);
        try (Cursor cursor = mContentResolver.query(recordedProgramsUri, null, null, null, null)) {
            assertEquals(0, cursor.getCount());
        }
    }

    public void testRecordedProgramsTable() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        // Set-up: add a channel.
        ContentValues values = createDummyChannelValues(mInputId);
        Uri channelUri = mContentResolver.insert(mChannelsUri, values);
        long channelId = ContentUris.parseId(channelUri);

        verifyRecordedProgramsTable(TvContract.RecordedPrograms.CONTENT_URI, channelId);
    }

    private void verifyQueryWithSortOrder(Uri uri, final String[] projection,
            String sortOrder) throws Exception {
        try {
            getContext().getContentResolver().query(uri, projection, null, null, sortOrder);
        } catch (SecurityException e) {
            fail("Setting sort order shoud be allowed for " + uri);
        }
    }

    private void verifyQueryWithSelection(Uri uri, final String[] projection,
            String selection) throws Exception {
        try {
            getContext().getContentResolver().query(uri, projection, selection, null, null);
            fail("Setting selection should fail without ACCESS_ALL_EPG_DATA permission for " + uri);
        } catch (SecurityException e) {
            // Expected exception
        }
    }

    private void verifyUpdateWithSelection(Uri uri, String selection) throws Exception {
        try {
            ContentValues values = new ContentValues();
            getContext().getContentResolver().update(uri, values, selection, null);
            fail("Setting selection should fail without ACCESS_ALL_EPG_DATA permission for " + uri);
        } catch (SecurityException e) {
            // Expected exception
        }
    }

    private void verifyDeleteWithSelection(Uri uri, String selection) throws Exception {
        try {
            getContext().getContentResolver().delete(uri, selection, null);
            fail("Setting selection should fail without ACCESS_ALL_EPG_DATA permission for " + uri);
        } catch (SecurityException e) {
            // Expected exception
        }
    }

    public void testAllEpgPermissionBlocksSortOrderOnQuery_Channels() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        final String[] projection = { TvContract.Channels._ID };
        verifyQueryWithSortOrder(TvContract.Channels.CONTENT_URI, projection,
                TvContract.Channels._ID + " ASC");
    }

    public void testAllEpgPermissionBlocksSelectionOnQuery_Channels() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        final String[] projection = { TvContract.Channels._ID };
        verifyQueryWithSelection(TvContract.Channels.CONTENT_URI, projection,
                TvContract.Channels._ID + ">0");
    }

    public void testAllEpgPermissionBlocksSelectionOnUpdate_Channels() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        verifyUpdateWithSelection(TvContract.Channels.CONTENT_URI,
                TvContract.Channels._ID + ">0");
    }

    public void testAllEpgPermissionBlocksSelectionOnDelete_Channels() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        verifyDeleteWithSelection(TvContract.Channels.CONTENT_URI,
                TvContract.Channels._ID + ">0");
    }

    public void testAllEpgPermissionBlocksSortOrderOnQuery_Programs() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        final String[] projection = { TvContract.Programs._ID };
        verifyQueryWithSortOrder(TvContract.Programs.CONTENT_URI, projection,
                TvContract.Programs._ID + " ASC");
    }

    public void testAllEpgPermissionBlocksSelectionOnQuery_Programs() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        final String[] projection = { TvContract.Channels._ID };
        verifyQueryWithSelection(TvContract.Programs.CONTENT_URI, projection,
                TvContract.Programs._ID + ">0");
    }

    public void testAllEpgPermissionBlocksSelectionOnUpdate_Programs() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        verifyUpdateWithSelection(TvContract.Programs.CONTENT_URI,
                TvContract.Programs._ID + ">0");
    }

    public void testAllEpgPermissionBlocksSelectionOnDelete_Programs() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        verifyDeleteWithSelection(TvContract.Programs.CONTENT_URI,
                TvContract.Programs._ID + ">0");
    }

    public void testDefaultValues() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        ContentValues values = new ContentValues();
        values.put(TvContract.Channels.COLUMN_INPUT_ID, mInputId);
        Uri channelUri = mContentResolver.insert(mChannelsUri, values);
        assertNotNull(channelUri);
        try (Cursor cursor = mContentResolver.query(
                channelUri, CHANNELS_PROJECTION, null, null, null)) {
            cursor.moveToNext();
            assertEquals(TvContract.Channels.TYPE_OTHER,
                    cursor.getString(cursor.getColumnIndex(TvContract.Channels.COLUMN_TYPE)));
            assertEquals(TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO,
                    cursor.getString(cursor.getColumnIndex(
                            TvContract.Channels.COLUMN_SERVICE_TYPE)));
        }
        values.clear();
    }

    public void testChannelsGetVideoResolution() {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        assertEquals(Channels.VIDEO_RESOLUTION_SD, Channels.getVideoResolution(
                Channels.VIDEO_FORMAT_480I));
        assertEquals(Channels.VIDEO_RESOLUTION_ED, Channels.getVideoResolution(
                Channels.VIDEO_FORMAT_480P));
        assertEquals(Channels.VIDEO_RESOLUTION_SD, Channels.getVideoResolution(
                Channels.VIDEO_FORMAT_576I));
        assertEquals(Channels.VIDEO_RESOLUTION_ED, Channels.getVideoResolution(
                Channels.VIDEO_FORMAT_576P));
        assertEquals(Channels.VIDEO_RESOLUTION_HD, Channels.getVideoResolution(
                Channels.VIDEO_FORMAT_720P));
        assertEquals(Channels.VIDEO_RESOLUTION_HD, Channels.getVideoResolution(
                Channels.VIDEO_FORMAT_1080I));
        assertEquals(Channels.VIDEO_RESOLUTION_FHD, Channels.getVideoResolution(
                Channels.VIDEO_FORMAT_1080P));
        assertEquals(Channels.VIDEO_RESOLUTION_UHD, Channels.getVideoResolution(
                Channels.VIDEO_FORMAT_2160P));
        assertEquals(Channels.VIDEO_RESOLUTION_UHD, Channels.getVideoResolution(
                Channels.VIDEO_FORMAT_4320P));
        assertEquals(null, Channels.getVideoResolution("Unknown format"));
    }

    public void testProgramsGenresDecode() {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        List genres = Arrays.asList(Genres.decode(ENCODED_GENRE_STRING));
        assertEquals(11, genres.size());
        assertTrue(genres.contains(Genres.ANIMAL_WILDLIFE));
        assertTrue(genres.contains(Genres.COMEDY));
        assertTrue(genres.contains(Genres.DRAMA));
        assertTrue(genres.contains(Genres.EDUCATION));
        assertTrue(genres.contains(Genres.FAMILY_KIDS));
        assertTrue(genres.contains(Genres.GAMING));
        assertTrue(genres.contains(Genres.MOVIES));
        assertTrue(genres.contains(Genres.NEWS));
        assertTrue(genres.contains(Genres.SHOPPING));
        assertTrue(genres.contains(Genres.SPORTS));
        assertTrue(genres.contains(Genres.TRAVEL));
        assertFalse(genres.contains(","));
    }

    public void testProgramsGenresEncode() {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        assertEquals(ENCODED_GENRE_STRING, Genres.encode(Genres.ANIMAL_WILDLIFE,
                Genres.COMEDY, Genres.DRAMA, Genres.EDUCATION, Genres.FAMILY_KIDS, Genres.GAMING,
                Genres.MOVIES, Genres.NEWS, Genres.SHOPPING, Genres.SPORTS, Genres.TRAVEL));
    }

    public void testProgramsGenresEncodeDecode_empty() {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        String[] genres = new String[] {EMPTY_GENRE};
        String expectedEncoded = EMPTY_GENRE;
        checkGenreEncodeDecode(genres, expectedEncoded, 0);

        genres = new String[] {EMPTY_GENRE, EMPTY_GENRE, EMPTY_GENRE};
        expectedEncoded = DELIMITER + DELIMITER;
        checkGenreEncodeDecode(genres, expectedEncoded, 0);
    }

    public void testProgramsGenresEncodeDecode_simpleDelimiter() {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        String[] genres = new String[] {EMPTY_GENRE,
                COMMA,
                QUOTE,
                COMMA + QUOTE,
                QUOTE + COMMA,
                COMMA + COMMA,
                QUOTE + QUOTE,
                COMMA + QUOTE + COMMA,
                QUOTE + COMMA + QUOTE};
        String expectedEncoded =
                DELIMITER + COMMA_ENCODED
                + DELIMITER + QUOTE_ENCODED
                + DELIMITER + COMMA_ENCODED + QUOTE_ENCODED
                + DELIMITER + QUOTE_ENCODED + COMMA_ENCODED
                + DELIMITER + COMMA_ENCODED + COMMA_ENCODED
                + DELIMITER + QUOTE_ENCODED + QUOTE_ENCODED
                + DELIMITER + COMMA_ENCODED + QUOTE_ENCODED + COMMA_ENCODED
                + DELIMITER + QUOTE_ENCODED + COMMA_ENCODED + QUOTE_ENCODED;
        checkGenreEncodeDecode(genres, expectedEncoded, genres.length - 1);
    }

    public void testProgramsGenresEncodeDecode_delimiterWithWhiteSpace() {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        String[] genres = new String[] {EMPTY_GENRE,
                COMMA + WHITE_SPACES,
                QUOTE + WHITE_SPACES,
                WHITE_SPACES + COMMA,
                WHITE_SPACES + QUOTE,
                WHITE_SPACES + COMMA + WHITE_SPACES,
                WHITE_SPACES + QUOTE + WHITE_SPACES};
        String expectedEncoded =
                DELIMITER + COMMA_ENCODED + WHITE_SPACES
                + DELIMITER + QUOTE_ENCODED + WHITE_SPACES
                + DELIMITER + WHITE_SPACES + COMMA_ENCODED
                + DELIMITER + WHITE_SPACES + QUOTE_ENCODED
                + DELIMITER + WHITE_SPACES + COMMA_ENCODED + WHITE_SPACES
                + DELIMITER + WHITE_SPACES + QUOTE_ENCODED + WHITE_SPACES;
        checkGenreEncodeDecode(genres, expectedEncoded, genres.length - 1);
    }

    public void testProgramsGenresEncodeDecode_all() {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        String[] genres = new String[] {EMPTY_GENRE,
                Genres.COMEDY,
                Genres.COMEDY + COMMA,
                Genres.COMEDY + COMMA + Genres.COMEDY,
                COMMA + Genres.COMEDY + COMMA,
                QUOTE + Genres.COMEDY + QUOTE,
                Genres.COMEDY + COMMA + WHITE_SPACES,
                Genres.COMEDY + COMMA + Genres.COMEDY + WHITE_SPACES,
                COMMA + Genres.COMEDY + COMMA + WHITE_SPACES,
                QUOTE + Genres.COMEDY + QUOTE + WHITE_SPACES
        };
        String expectedEncoded =
                DELIMITER + Genres.COMEDY
                + DELIMITER + Genres.COMEDY + COMMA_ENCODED
                + DELIMITER + Genres.COMEDY + COMMA_ENCODED + Genres.COMEDY
                + DELIMITER + COMMA_ENCODED + Genres.COMEDY + COMMA_ENCODED
                + DELIMITER + QUOTE_ENCODED + Genres.COMEDY + QUOTE_ENCODED
                + DELIMITER + Genres.COMEDY + COMMA_ENCODED + WHITE_SPACES
                + DELIMITER + Genres.COMEDY + COMMA_ENCODED + Genres.COMEDY + WHITE_SPACES
                + DELIMITER + COMMA_ENCODED + Genres.COMEDY + COMMA_ENCODED + WHITE_SPACES
                + DELIMITER + QUOTE_ENCODED + Genres.COMEDY + QUOTE_ENCODED + WHITE_SPACES;
        checkGenreEncodeDecode(genres, expectedEncoded, genres.length - 1);
    }

    private void checkGenreEncodeDecode(String[] genres, String expectedEncoded,
            int expectedDecodedLength) {
        String encoded = Genres.encode(genres);
        assertEquals(expectedEncoded, encoded);
        String[] decoded = Genres.decode(encoded);
        assertEquals(expectedDecodedLength, decoded.length);
        int decodedIndex = 0;
        for (int i = 0; i < genres.length; ++i) {
            String original = genres[i].trim();
            if (!original.isEmpty()) {
                assertEquals(original, decoded[decodedIndex++]);
            }
        }
    }

    private Uri insertProgramWithBroadcastGenre(String[] broadcastGenre) {
        ContentValues values = createDummyChannelValues(mInputId);
        Uri channelUri = mContentResolver.insert(Channels.CONTENT_URI, values);
        long channelId = ContentUris.parseId(channelUri);
        long curTime = System.currentTimeMillis();
        values = new ContentValues();
        values.put(TvContract.Programs.COLUMN_CHANNEL_ID, channelId);
        values.put(TvContract.Programs.COLUMN_BROADCAST_GENRE, Genres.encode(broadcastGenre));
        values.put(TvContract.Programs.COLUMN_START_TIME_UTC_MILLIS, curTime - 60000);
        values.put(TvContract.Programs.COLUMN_END_TIME_UTC_MILLIS, curTime + 60000);
        Uri programUri = mContentResolver.insert(TvContract.Programs.CONTENT_URI, values);
        assertNotNull(programUri);
        return programUri;
    }

    private void verifyChannelCountWithCanonicalGenre(String canonicalGenre, int expectedCount) {
        Uri channelUri = TvContract.buildChannelsUriForInput(mInputId).buildUpon()
                .appendQueryParameter(PARAM_CANONICAL_GENRE, canonicalGenre).build();
        try (Cursor c = mContentResolver.query(channelUri, new String[] {Channels._ID}, null, null,
                null)) {
            assertNotNull(c);
            assertEquals("Query:{Uri=" + channelUri + "}", expectedCount, c.getCount());
        }
    }

    public void testBroadcastGenreEncodeDecode() {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        String[] broadcastGenre = new String[] {"Animation", "Classic, opera"};
        insertProgramWithBroadcastGenre(broadcastGenre);
        try (Cursor c = mContentResolver.query(TvContract.Programs.CONTENT_URI,
                new String[] {TvContract.Programs.COLUMN_BROADCAST_GENRE}, null, null, null)) {
            assertNotNull(c);
            assertEquals(1, c.getCount());
            c.moveToNext();
            MoreAsserts.assertEquals(broadcastGenre, Genres.decode(c.getString(0)));
        }
    }

    public void testBroadcastGenreQueryChannel() {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        // "Animation" is mapped to Genres.MOVIES
        // "Classic, opera" is mapped to Genres.MUSIC
        insertProgramWithBroadcastGenre(new String[]{"Animation"});
        insertProgramWithBroadcastGenre(new String[] {"Classic, opera"});
        insertProgramWithBroadcastGenre(new String[]{"Animation", "Classic, opera"});
        // There are two channels which belong to MOVIES genre - channel 1 and 3.
        verifyChannelCountWithCanonicalGenre(Genres.MOVIES, 2);
        // There are two channels which belong to MUSIC genre - channel 2 and 3.
        verifyChannelCountWithCanonicalGenre(Genres.MUSIC, 2);
    }

    public void testGenresIsCanonical() {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        assertTrue(Genres.isCanonical(Genres.DRAMA));
        assertFalse(Genres.isCanonical("Not a genre"));
    }

    public void testUriUtils() {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        final Uri CHANNEL_URI_FOR_TUNER = TvContract.buildChannelUri(0);
        final Uri CHANNEL_URI_FOR_PASSTHROUGH_INPUT =
                TvContract.buildChannelUriForPassthroughInput("inputId");
        final Uri PROGRAM_URI = TvContract.buildProgramUri(0);

        // Test isChannelUri
        assertTrue(TvContract.isChannelUri(CHANNEL_URI_FOR_TUNER));
        assertTrue(TvContract.isChannelUri(CHANNEL_URI_FOR_PASSTHROUGH_INPUT));
        assertFalse(TvContract.isChannelUri(PROGRAM_URI));
        assertFalse(TvContract.isChannelUri(null));

        // Test isChannelUriForPassthroughInput
        assertFalse(TvContract.isChannelUriForPassthroughInput(CHANNEL_URI_FOR_TUNER));
        assertTrue(TvContract.isChannelUriForPassthroughInput(CHANNEL_URI_FOR_PASSTHROUGH_INPUT));
        assertFalse(TvContract.isChannelUriForPassthroughInput(PROGRAM_URI));
        assertFalse(TvContract.isChannelUriForPassthroughInput(null));

        // Test isChannelUriForTunerInput
        assertTrue(TvContract.isChannelUriForTunerInput(CHANNEL_URI_FOR_TUNER));
        assertFalse(TvContract.isChannelUriForTunerInput(CHANNEL_URI_FOR_PASSTHROUGH_INPUT));
        assertFalse(TvContract.isChannelUriForTunerInput(PROGRAM_URI));
        assertFalse(TvContract.isChannelUriForTunerInput(null));

        // Test isProgramUri
        assertFalse(TvContract.isProgramUri(CHANNEL_URI_FOR_TUNER));
        assertFalse(TvContract.isProgramUri(CHANNEL_URI_FOR_PASSTHROUGH_INPUT));
        assertTrue(TvContract.isProgramUri(PROGRAM_URI));
        assertFalse(TvContract.isProgramUri(null));
    }
}
