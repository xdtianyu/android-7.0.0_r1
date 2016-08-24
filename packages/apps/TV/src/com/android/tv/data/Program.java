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

import android.content.Context;
import android.database.Cursor;
import android.media.tv.TvContentRating;
import android.media.tv.TvContract;
import android.support.annotation.NonNull;
import android.support.annotation.UiThread;
import android.support.v4.os.BuildCompat;
import android.text.TextUtils;
import android.util.Log;

import com.android.tv.R;
import com.android.tv.common.BuildConfig;
import com.android.tv.common.CollectionUtils;
import com.android.tv.common.TvContentRatingCache;
import com.android.tv.util.ImageLoader;
import com.android.tv.util.Utils;

import java.util.Arrays;
import java.util.Objects;

/**
 * A convenience class to create and insert program information entries into the database.
 */
public final class Program implements Comparable<Program> {
    private static final boolean DEBUG = false;
    private static final boolean DEBUG_DUMP_DESCRIPTION = false;
    private static final String TAG = "Program";

    private static final String[] PROJECTION_BASE = {
            // Columns must match what is read in Program.fromCursor()
            TvContract.Programs._ID,
            TvContract.Programs.COLUMN_CHANNEL_ID,
            TvContract.Programs.COLUMN_TITLE,
            TvContract.Programs.COLUMN_EPISODE_TITLE,
            TvContract.Programs.COLUMN_SHORT_DESCRIPTION,
            TvContract.Programs.COLUMN_POSTER_ART_URI,
            TvContract.Programs.COLUMN_THUMBNAIL_URI,
            TvContract.Programs.COLUMN_CANONICAL_GENRE,
            TvContract.Programs.COLUMN_CONTENT_RATING,
            TvContract.Programs.COLUMN_START_TIME_UTC_MILLIS,
            TvContract.Programs.COLUMN_END_TIME_UTC_MILLIS,
            TvContract.Programs.COLUMN_VIDEO_WIDTH,
            TvContract.Programs.COLUMN_VIDEO_HEIGHT
    };

    // Columns which is deprecated in NYC
    private static final String[] PROJECTION_DEPRECATED_IN_NYC = {
            TvContract.Programs.COLUMN_SEASON_NUMBER,
            TvContract.Programs.COLUMN_EPISODE_NUMBER
    };

    private static final String[] PROJECTION_ADDED_IN_NYC = {
            TvContract.Programs.COLUMN_SEASON_DISPLAY_NUMBER,
            TvContract.Programs.COLUMN_SEASON_TITLE,
            TvContract.Programs.COLUMN_EPISODE_DISPLAY_NUMBER
    };

    public static final String[] PROJECTION = createProjection();

    private static String[] createProjection() {
        return CollectionUtils
                .concatAll(PROJECTION_BASE, BuildCompat.isAtLeastN() ? PROJECTION_ADDED_IN_NYC
                : PROJECTION_DEPRECATED_IN_NYC);
    }

    /**
     * Creates {@code Program} object from cursor.
     *
     * <p>The query that created the cursor MUST use {@link #PROJECTION}.
     */
    public static Program fromCursor(Cursor cursor) {
        // Columns read must match the order of match {@link #PROJECTION}
        Builder builder = new Builder();
        int index = 0;
        builder.setId(cursor.getLong(index++));
        builder.setChannelId(cursor.getLong(index++));
        builder.setTitle(cursor.getString(index++));
        builder.setEpisodeTitle(cursor.getString(index++));
        builder.setDescription(cursor.getString(index++));
        builder.setPosterArtUri(cursor.getString(index++));
        builder.setThumbnailUri(cursor.getString(index++));
        builder.setCanonicalGenres(cursor.getString(index++));
        builder.setContentRatings(
                TvContentRatingCache.getInstance().getRatings(cursor.getString(index++)));
        builder.setStartTimeUtcMillis(cursor.getLong(index++));
        builder.setEndTimeUtcMillis(cursor.getLong(index++));
        builder.setVideoWidth((int) cursor.getLong(index++));
        builder.setVideoHeight((int) cursor.getLong(index++));
        if (BuildCompat.isAtLeastN()) {
            builder.setSeasonNumber(cursor.getString(index++));
            builder.setSeasonTitle(cursor.getString(index++));
            builder.setEpisodeNumber(cursor.getString(index++));
        } else {
            builder.setSeasonNumber(cursor.getString(index++));
            builder.setEpisodeNumber(cursor.getString(index++));
        }
        return builder.build();
    }

    private long mId;
    private long mChannelId;
    private String mTitle;
    private String mEpisodeTitle;
    private String mSeasonNumber;
    private String mSeasonTitle;
    private String mEpisodeNumber;
    private long mStartTimeUtcMillis;
    private long mEndTimeUtcMillis;
    private String mDescription;
    private int mVideoWidth;
    private int mVideoHeight;
    private String mPosterArtUri;
    private String mThumbnailUri;
    private int[] mCanonicalGenreIds;
    private TvContentRating[] mContentRatings;

    /**
     * TODO(DVR): Need to fill the following data.
     */
    private boolean mRecordable;
    private boolean mRecordingScheduled;

    private Program() {
        // Do nothing.
    }

    public long getId() {
        return mId;
    }

    public long getChannelId() {
        return mChannelId;
    }

    /**
     * Returns {@code true} if this program is valid or {@code false} otherwise.
     */
    public boolean isValid() {
        return mChannelId >= 0;
    }

    /**
     * Returns {@code true} if the program is valid and {@code false} otherwise.
     */
    public static boolean isValid(Program program) {
        return program != null && program.isValid();
    }

    public String getTitle() {
        return mTitle;
    }

    public String getEpisodeTitle() {
        return mEpisodeTitle;
    }

    public String getEpisodeDisplayTitle(Context context) {
        if (!TextUtils.isEmpty(mSeasonNumber) && !TextUtils.isEmpty(mEpisodeNumber)
                && !TextUtils.isEmpty(mEpisodeTitle)) {
            return String.format(context.getResources().getString(R.string.episode_format),
                    mSeasonNumber, mEpisodeNumber, mEpisodeTitle);
        }
        return mEpisodeTitle;
    }

    public String getSeasonNumber() {
        return mSeasonNumber;
    }

    public String getEpisodeNumber() {
        return mEpisodeNumber;
    }

    public long getStartTimeUtcMillis() {
        return mStartTimeUtcMillis;
    }

    public long getEndTimeUtcMillis() {
        return mEndTimeUtcMillis;
    }

    /**
     * Returns the program duration.
     */
    public long getDurationMillis() {
        return mEndTimeUtcMillis - mStartTimeUtcMillis;
    }

    public String getDescription() {
        return mDescription;
    }

    public int getVideoWidth() {
        return mVideoWidth;
    }

    public int getVideoHeight() {
        return mVideoHeight;
    }

    public TvContentRating[] getContentRatings() {
        return mContentRatings;
    }

    public String getPosterArtUri() {
        return mPosterArtUri;
    }

    public String getThumbnailUri() {
        return mThumbnailUri;
    }

    /**
     * Returns array of canonical genres for this program.
     * This is expected to be called rarely.
     */
    public String[] getCanonicalGenres() {
        if (mCanonicalGenreIds == null) {
            return null;
        }
        String[] genres = new String[mCanonicalGenreIds.length];
        for (int i = 0; i < mCanonicalGenreIds.length; i++) {
            genres[i] = GenreItems.getCanonicalGenre(mCanonicalGenreIds[i]);
        }
        return genres;
    }

    /**
     * Returns if this program has the genre.
     */
    public boolean hasGenre(int genreId) {
        if (genreId == GenreItems.ID_ALL_CHANNELS) {
            return true;
        }
        if (mCanonicalGenreIds != null) {
            for (int id : mCanonicalGenreIds) {
                if (id == genreId) {
                    return true;
                }
            }
        }
        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(mChannelId, mStartTimeUtcMillis, mEndTimeUtcMillis,
                mTitle, mEpisodeTitle, mDescription, mVideoWidth, mVideoHeight,
                mPosterArtUri, mThumbnailUri, Arrays.hashCode(mContentRatings),
                Arrays.hashCode(mCanonicalGenreIds), mSeasonNumber, mSeasonTitle, mEpisodeNumber);
    }

    @Override
    public boolean equals(Object other) {
        if (!(other instanceof Program)) {
            return false;
        }
        Program program = (Program) other;
        return mChannelId == program.mChannelId
                && mStartTimeUtcMillis == program.mStartTimeUtcMillis
                && mEndTimeUtcMillis == program.mEndTimeUtcMillis
                && Objects.equals(mTitle, program.mTitle)
                && Objects.equals(mEpisodeTitle, program.mEpisodeTitle)
                && Objects.equals(mDescription, program.mDescription)
                && mVideoWidth == program.mVideoWidth
                && mVideoHeight == program.mVideoHeight
                && Objects.equals(mPosterArtUri, program.mPosterArtUri)
                && Objects.equals(mThumbnailUri, program.mThumbnailUri)
                && Arrays.equals(mContentRatings, program.mContentRatings)
                && Arrays.equals(mCanonicalGenreIds, program.mCanonicalGenreIds)
                && Objects.equals(mSeasonNumber, program.mSeasonNumber)
                && Objects.equals(mSeasonTitle, program.mSeasonTitle)
                && Objects.equals(mEpisodeNumber, program.mEpisodeNumber);
    }

    @Override
    public int compareTo(@NonNull Program other) {
        return Long.compare(mStartTimeUtcMillis, other.mStartTimeUtcMillis);
    }

    @Override
    public String toString() {
        StringBuilder builder = new StringBuilder();
        builder.append("Program[" + mId + "]{")
                .append("channelId=").append(mChannelId)
                .append(", title=").append(mTitle)
                .append(", episodeTitle=").append(mEpisodeTitle)
                .append(", seasonNumber=").append(mSeasonNumber)
                .append(", seasonTitle=").append(mSeasonTitle)
                .append(", episodeNumber=").append(mEpisodeNumber)
                .append(", startTimeUtcSec=").append(Utils.toTimeString(mStartTimeUtcMillis))
                .append(", endTimeUtcSec=").append(Utils.toTimeString(mEndTimeUtcMillis))
                .append(", videoWidth=").append(mVideoWidth)
                .append(", videoHeight=").append(mVideoHeight)
                .append(", contentRatings=")
                .append(TvContentRatingCache.contentRatingsToString(mContentRatings))
                .append(", posterArtUri=").append(mPosterArtUri)
                .append(", thumbnailUri=").append(mThumbnailUri)
                .append(", canonicalGenres=").append(Arrays.toString(mCanonicalGenreIds));
        if (DEBUG_DUMP_DESCRIPTION) {
            builder.append(", description=").append(mDescription);
        }
        return builder.append("}").toString();
    }

    public void copyFrom(Program other) {
        if (this == other) {
            return;
        }

        mId = other.mId;
        mChannelId = other.mChannelId;
        mTitle = other.mTitle;
        mEpisodeTitle = other.mEpisodeTitle;
        mSeasonNumber = other.mSeasonNumber;
        mSeasonTitle = other.mSeasonTitle;
        mEpisodeNumber = other.mEpisodeNumber;
        mStartTimeUtcMillis = other.mStartTimeUtcMillis;
        mEndTimeUtcMillis = other.mEndTimeUtcMillis;
        mDescription = other.mDescription;
        mVideoWidth = other.mVideoWidth;
        mVideoHeight = other.mVideoHeight;
        mPosterArtUri = other.mPosterArtUri;
        mThumbnailUri = other.mThumbnailUri;
        mCanonicalGenreIds = other.mCanonicalGenreIds;
        mContentRatings = other.mContentRatings;
    }

    public static final class Builder {
        private final Program mProgram;
        private long mId;

        public Builder() {
            mProgram = new Program();
            // Fill initial data.
            mProgram.mChannelId = Channel.INVALID_ID;
            mProgram.mTitle = null;
            mProgram.mSeasonNumber = null;
            mProgram.mSeasonTitle = null;
            mProgram.mEpisodeNumber = null;
            mProgram.mStartTimeUtcMillis = -1;
            mProgram.mEndTimeUtcMillis = -1;
            mProgram.mDescription = null;
        }

        public Builder(Program other) {
            mProgram = new Program();
            mProgram.copyFrom(other);
        }

        public Builder setId(long id) {
            mProgram.mId = id;
            return this;
        }

        public Builder setChannelId(long channelId) {
            mProgram.mChannelId = channelId;
            return this;
        }

        public Builder setTitle(String title) {
            mProgram.mTitle = title;
            return this;
        }

        public Builder setEpisodeTitle(String episodeTitle) {
            mProgram.mEpisodeTitle = episodeTitle;
            return this;
        }

        public Builder setSeasonNumber(String seasonNumber) {
            mProgram.mSeasonNumber = seasonNumber;
            return this;
        }

        public Builder setSeasonTitle(String seasonTitle) {
            mProgram.mSeasonTitle = seasonTitle;
            return this;
        }

        public Builder setEpisodeNumber(String episodeNumber) {
            mProgram.mEpisodeNumber = episodeNumber;
            return this;
        }

        public Builder setStartTimeUtcMillis(long startTimeUtcMillis) {
            mProgram.mStartTimeUtcMillis = startTimeUtcMillis;
            return this;
        }

        public Builder setEndTimeUtcMillis(long endTimeUtcMillis) {
            mProgram.mEndTimeUtcMillis = endTimeUtcMillis;
            return this;
        }

        public Builder setDescription(String description) {
            mProgram.mDescription = description;
            return this;
        }

        public Builder setVideoWidth(int width) {
            mProgram.mVideoWidth = width;
            return this;
        }

        public Builder setVideoHeight(int height) {
            mProgram.mVideoHeight = height;
            return this;
        }

        public Builder setContentRatings(TvContentRating[] contentRatings) {
            mProgram.mContentRatings = contentRatings;
            return this;
        }

        public Builder setPosterArtUri(String posterArtUri) {
            mProgram.mPosterArtUri = posterArtUri;
            return this;
        }

        public Builder setThumbnailUri(String thumbnailUri) {
            mProgram.mThumbnailUri = thumbnailUri;
            return this;
        }

        public Builder setCanonicalGenres(String genres) {
            if (TextUtils.isEmpty(genres)) {
                return this;
            }
            String[] canonicalGenres = TvContract.Programs.Genres.decode(genres);
            if (canonicalGenres.length > 0) {
                int[] temp = new int[canonicalGenres.length];
                int i = 0;
                for (String canonicalGenre : canonicalGenres) {
                    int genreId = GenreItems.getId(canonicalGenre);
                    if (genreId == GenreItems.ID_ALL_CHANNELS) {
                        // Skip if the genre is unknown.
                        continue;
                    }
                    temp[i++] = genreId;
                }
                if (i < canonicalGenres.length) {
                    temp = Arrays.copyOf(temp, i);
                }
                mProgram.mCanonicalGenreIds=temp;
            }
            return this;
        }

        public Program build() {
            Program program = new Program();
            program.copyFrom(mProgram);
            return program;
        }
    }

    /**
     * Prefetches the program poster art.<p>
     */
    public void prefetchPosterArt(Context context, int posterArtWidth, int posterArtHeight) {
        if (mPosterArtUri == null) {
            return;
        }
        ImageLoader.prefetchBitmap(context, mPosterArtUri, posterArtWidth, posterArtHeight);
    }

    /**
     * Loads the program poster art and returns it via {@code callback}.<p>
     * <p>
     * Note that it may directly call {@code callback} if the program poster art already is loaded.
     */
    @UiThread
    public void loadPosterArt(Context context, int posterArtWidth, int posterArtHeight,
            ImageLoader.ImageLoaderCallback callback) {
        if (mPosterArtUri == null) {
            return;
        }
        ImageLoader.loadBitmap(context, mPosterArtUri, posterArtWidth, posterArtHeight, callback);
    }

    public static boolean isDuplicate(Program p1, Program p2) {
        if (p1 == null || p2 == null) {
            return false;
        }
        boolean isDuplicate = p1.getChannelId() == p2.getChannelId()
                && p1.getStartTimeUtcMillis() == p2.getStartTimeUtcMillis()
                && p1.getEndTimeUtcMillis() == p2.getEndTimeUtcMillis();
        if (DEBUG && BuildConfig.ENG && isDuplicate) {
            Log.w(TAG, "Duplicate programs detected! - \"" + p1.getTitle() + "\" and \""
                    + p2.getTitle() + "\"");
        }
        return isDuplicate;
    }
}
