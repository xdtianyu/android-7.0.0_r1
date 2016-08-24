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
 * limitations under the License
 */

package com.android.tv.common.recording;

import static android.media.tv.TvContract.RecordedPrograms;

import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.media.tv.TvContract;
import android.net.Uri;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import com.android.tv.common.R;

import java.util.Arrays;
import java.util.Comparator;
import java.util.Objects;

/**
 * Immutable instance of {@link android.media.tv.TvContract.RecordedPrograms}.
 */
public class RecordedProgram {
    public static final int ID_NOT_SET = -1;

    public final static String[] PROJECTION = {
            // These are in exactly the order listed in RecordedPrograms
            RecordedPrograms._ID,
            RecordedPrograms.COLUMN_INPUT_ID,
            RecordedPrograms.COLUMN_CHANNEL_ID,
            RecordedPrograms.COLUMN_TITLE,
            RecordedPrograms.COLUMN_SEASON_DISPLAY_NUMBER,
            RecordedPrograms.COLUMN_SEASON_TITLE,
            RecordedPrograms.COLUMN_EPISODE_DISPLAY_NUMBER,
            RecordedPrograms.COLUMN_EPISODE_TITLE,
            RecordedPrograms.COLUMN_START_TIME_UTC_MILLIS,
            RecordedPrograms.COLUMN_END_TIME_UTC_MILLIS,
            RecordedPrograms.COLUMN_BROADCAST_GENRE,
            RecordedPrograms.COLUMN_CANONICAL_GENRE,
            RecordedPrograms.COLUMN_SHORT_DESCRIPTION,
            RecordedPrograms.COLUMN_LONG_DESCRIPTION,
            RecordedPrograms.COLUMN_VIDEO_WIDTH,
            RecordedPrograms.COLUMN_VIDEO_HEIGHT,
            RecordedPrograms.COLUMN_AUDIO_LANGUAGE,
            RecordedPrograms.COLUMN_CONTENT_RATING,
            RecordedPrograms.COLUMN_POSTER_ART_URI,
            RecordedPrograms.COLUMN_THUMBNAIL_URI,
            RecordedPrograms.COLUMN_SEARCHABLE,
            RecordedPrograms.COLUMN_RECORDING_DATA_URI,
            RecordedPrograms.COLUMN_RECORDING_DATA_BYTES,
            RecordedPrograms.COLUMN_RECORDING_DURATION_MILLIS,
            RecordedPrograms.COLUMN_RECORDING_EXPIRE_TIME_UTC_MILLIS,
            RecordedPrograms.COLUMN_INTERNAL_PROVIDER_DATA,
            RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG1,
            RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG2,
            RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG3,
            RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG4,
            RecordedPrograms.COLUMN_VERSION_NUMBER,
    };

    public static final RecordedProgram fromCursor(Cursor cursor) {
        int index = 0;
        return builder()
                .setId(cursor.getLong(index++))
                .setInputId(cursor.getString(index++))
                .setChannelId(cursor.getLong(index++))
                .setTitle(cursor.getString(index++))
                .setSeasonNumber(cursor.getString(index++))
                .setSeasonTitle(cursor.getString(index++))
                .setEpisodeNumber(cursor.getString(index++))
                .setEpisodeTitle(cursor.getString(index++))
                .setStartTimeUtcMillis(cursor.getLong(index++))
                .setEndTimeUtcMillis(cursor.getLong(index++))
                .setBroadcastGenres(cursor.getString(index++))
                .setCanonicalGenres(cursor.getString(index++))
                .setShortDescription(cursor.getString(index++))
                .setLongDescription(cursor.getString(index++))
                .setVideoWidth(cursor.getInt(index++))
                .setVideoHeight(cursor.getInt(index++))
                .setAudioLanguage(cursor.getString(index++))
                .setContentRating(cursor.getString(index++))
                .setPosterArt(cursor.getString(index++))
                .setThumbnail(cursor.getString(index++))
                .setSearchable(cursor.getInt(index++) == 1)
                .setDataUri(cursor.getString(index++))
                .setDataBytes(cursor.getLong(index++))
                .setDurationMillis(cursor.getLong(index++))
                .setExpireTimeUtcMillis(cursor.getLong(index++))
                .setInternalProviderData(cursor.getBlob(index++))
                .setInternalProviderFlag1(cursor.getInt(index++))
                .setInternalProviderFlag2(cursor.getInt(index++))
                .setInternalProviderFlag3(cursor.getInt(index++))
                .setInternalProviderFlag4(cursor.getInt(index++))
                .setVersionNumber(cursor.getInt(index++))
                .build();
    }

    public static ContentValues toValues(RecordedProgram recordedProgram) {
        ContentValues values = new ContentValues();
        if (recordedProgram.mId != ID_NOT_SET) {
            values.put(RecordedPrograms._ID, recordedProgram.mId);
        }
        values.put(RecordedPrograms.COLUMN_INPUT_ID, recordedProgram.mInputId);
        values.put(RecordedPrograms.COLUMN_CHANNEL_ID, recordedProgram.mChannelId);
        values.put(RecordedPrograms.COLUMN_TITLE, recordedProgram.mTitle);
        values.put(RecordedPrograms.COLUMN_SEASON_DISPLAY_NUMBER, recordedProgram.mSeasonNumber);
        values.put(RecordedPrograms.COLUMN_SEASON_TITLE, recordedProgram.mSeasonTitle);
        values.put(RecordedPrograms.COLUMN_EPISODE_DISPLAY_NUMBER, recordedProgram.mEpisodeNumber);
        values.put(RecordedPrograms.COLUMN_EPISODE_TITLE, recordedProgram.mTitle);
        values.put(RecordedPrograms.COLUMN_START_TIME_UTC_MILLIS,
                recordedProgram.mStartTimeUtcMillis);
        values.put(RecordedPrograms.COLUMN_END_TIME_UTC_MILLIS, recordedProgram.mEndTimeUtcMillis);
        values.put(RecordedPrograms.COLUMN_BROADCAST_GENRE,
                safeEncode(recordedProgram.mBroadcastGenres));
        values.put(RecordedPrograms.COLUMN_CANONICAL_GENRE,
                safeEncode(recordedProgram.mCanonicalGenres));
        values.put(RecordedPrograms.COLUMN_SHORT_DESCRIPTION, recordedProgram.mShortDescription);
        values.put(RecordedPrograms.COLUMN_LONG_DESCRIPTION, recordedProgram.mLongDescription);
        if (recordedProgram.mVideoWidth == 0) {
            values.putNull(RecordedPrograms.COLUMN_VIDEO_WIDTH);
        } else {
            values.put(RecordedPrograms.COLUMN_VIDEO_WIDTH, recordedProgram.mVideoWidth);
        }
        if (recordedProgram.mVideoHeight == 0) {
            values.putNull(RecordedPrograms.COLUMN_VIDEO_HEIGHT);
        } else {
            values.put(RecordedPrograms.COLUMN_VIDEO_HEIGHT, recordedProgram.mVideoHeight);
        }
        values.put(RecordedPrograms.COLUMN_AUDIO_LANGUAGE, recordedProgram.mAudioLanguage);
        values.put(RecordedPrograms.COLUMN_CONTENT_RATING, recordedProgram.mContentRating);
        values.put(RecordedPrograms.COLUMN_POSTER_ART_URI,
                safeToString(recordedProgram.mPosterArt));
        values.put(RecordedPrograms.COLUMN_THUMBNAIL_URI, safeToString(recordedProgram.mThumbnail));
        values.put(RecordedPrograms.COLUMN_SEARCHABLE, recordedProgram.mSearchable ? 1 : 0);
        values.put(RecordedPrograms.COLUMN_RECORDING_DATA_URI,
                safeToString(recordedProgram.mDataUri));
        values.put(RecordedPrograms.COLUMN_RECORDING_DATA_BYTES, recordedProgram.mDataBytes);
        values.put(RecordedPrograms.COLUMN_RECORDING_DURATION_MILLIS,
                recordedProgram.mDurationMillis);
        values.put(RecordedPrograms.COLUMN_RECORDING_EXPIRE_TIME_UTC_MILLIS,
                recordedProgram.mExpireTimeUtcMillis);
        values.put(RecordedPrograms.COLUMN_INTERNAL_PROVIDER_DATA,
                recordedProgram.mInternalProviderData);
        values.put(RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG1,
                recordedProgram.mInternalProviderFlag1);
        values.put(RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG2,
                recordedProgram.mInternalProviderFlag2);
        values.put(RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG3,
                recordedProgram.mInternalProviderFlag3);
        values.put(RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG4,
                recordedProgram.mInternalProviderFlag4);
        values.put(RecordedPrograms.COLUMN_VERSION_NUMBER, recordedProgram.mVersionNumber);
        return values;
    }

    public static class Builder{
        private long mId = ID_NOT_SET;
        private String mInputId;
        private long mChannelId;
        private String mTitle;
        private String mSeasonNumber;
        private String mSeasonTitle;
        private String mEpisodeNumber;
        private String mEpisodeTitle;
        private long mStartTimeUtcMillis;
        private long mEndTimeUtcMillis;
        private String[] mBroadcastGenres;
        private String[] mCanonicalGenres;
        private String mShortDescription;
        private String mLongDescription;
        private int mVideoWidth;
        private int mVideoHeight;
        private String mAudioLanguage;
        private String mContentRating;
        private Uri mPosterArt;
        private Uri mThumbnail;
        private boolean mSearchable = true;
        private Uri mDataUri;
        private long mDataBytes;
        private long mDurationMillis;
        private long mExpireTimeUtcMillis;
        private byte[] mInternalProviderData;
        private int mInternalProviderFlag1;
        private int mInternalProviderFlag2;
        private int mInternalProviderFlag3;
        private int mInternalProviderFlag4;
        private int mVersionNumber;

        public Builder setId(long id) {
            mId = id;
            return this;
        }

        public Builder setInputId(String inputId) {
            mInputId = inputId;
            return this;
        }

        public Builder setChannelId(long channelId) {
            mChannelId = channelId;
            return this;
        }

        public Builder setTitle(String title) {
            mTitle = title;
            return this;
        }

        public Builder setSeasonNumber(String seasonNumber) {
            mSeasonNumber = seasonNumber;
            return this;
        }

        public Builder setSeasonTitle(String seasonTitle) {
            mSeasonTitle = seasonTitle;
            return this;
        }

        public Builder setEpisodeNumber(String episodeNumber) {
            mEpisodeNumber = episodeNumber;
            return this;
        }

        public Builder setEpisodeTitle(String episodeTitle) {
            mEpisodeTitle = episodeTitle;
            return this;
        }

        public Builder setStartTimeUtcMillis(long startTimeUtcMillis) {
            mStartTimeUtcMillis = startTimeUtcMillis;
            return this;
        }

        public Builder setEndTimeUtcMillis(long endTimeUtcMillis) {
            mEndTimeUtcMillis = endTimeUtcMillis;
            return this;
        }

        public Builder setBroadcastGenres(String broadcastGenres) {
            if (TextUtils.isEmpty(broadcastGenres)) {
                mBroadcastGenres = null;
                return this;
            }
            return setBroadcastGenres(TvContract.Programs.Genres.decode(broadcastGenres));
        }

        private Builder setBroadcastGenres(String[] broadcastGenres) {
            mBroadcastGenres = broadcastGenres;
            return this;
        }

        public Builder setCanonicalGenres(String canonicalGenres) {
            if (TextUtils.isEmpty(canonicalGenres)) {
                mCanonicalGenres = null;
                return this;
            }
            return setCanonicalGenres(TvContract.Programs.Genres.decode(canonicalGenres));
        }

        private Builder setCanonicalGenres(String[] canonicalGenres) {
            mCanonicalGenres = canonicalGenres;
            return this;
        }

        public Builder setShortDescription(String shortDescription) {
            mShortDescription = shortDescription;
            return this;
        }

        public Builder setLongDescription(String longDescription) {
            mLongDescription = longDescription;
            return this;
        }

        public Builder setVideoWidth(int videoWidth) {
            mVideoWidth = videoWidth;
            return this;
        }

        public Builder setVideoHeight(int videoHeight) {
            mVideoHeight = videoHeight;
            return this;
        }

        public Builder setAudioLanguage(String audioLanguage) {
            mAudioLanguage = audioLanguage;
            return this;
        }

        public Builder setContentRating(String contentRating) {
            mContentRating = contentRating;
            return this;
        }

        private Uri toUri(String uriString) {
            try {
                return uriString == null ? null : Uri.parse(uriString);
            } catch (Exception e) {
                return null;
            }
        }

        public Builder setPosterArt(String posterArtUri) {
            return setPosterArt(toUri(posterArtUri));
        }

        public Builder setPosterArt(Uri posterArt) {
            mPosterArt = posterArt;
            return this;
        }

        public Builder setThumbnail(String thumbnailUri) {
            return setThumbnail(toUri(thumbnailUri));
        }

        public Builder setThumbnail(Uri thumbnail) {
            mThumbnail = thumbnail;
            return this;
        }

        public Builder setSearchable(boolean searchable) {
            mSearchable = searchable;
            return this;
        }

        public Builder setDataUri(String dataUri) {
            return setDataUri(toUri(dataUri));
        }

        public Builder setDataUri(Uri dataUri) {
            mDataUri = dataUri;
            return this;
        }

        public Builder setDataBytes(long dataBytes) {
            mDataBytes = dataBytes;
            return this;
        }

        public Builder setDurationMillis(long durationMillis) {
            mDurationMillis = durationMillis;
            return this;
        }

        public Builder setExpireTimeUtcMillis(long expireTimeUtcMillis) {
            mExpireTimeUtcMillis = expireTimeUtcMillis;
            return this;
        }

        public Builder setInternalProviderData(byte[] internalProviderData) {
            mInternalProviderData = internalProviderData;
            return this;
        }

        public Builder setInternalProviderFlag1(int internalProviderFlag1) {
            mInternalProviderFlag1 = internalProviderFlag1;
            return this;
        }

        public Builder setInternalProviderFlag2(int internalProviderFlag2) {
            mInternalProviderFlag2 = internalProviderFlag2;
            return this;
        }

        public Builder setInternalProviderFlag3(int internalProviderFlag3) {
            mInternalProviderFlag3 = internalProviderFlag3;
            return this;
        }

        public Builder setInternalProviderFlag4(int internalProviderFlag4) {
            mInternalProviderFlag4 = internalProviderFlag4;
            return this;
        }

        public Builder setVersionNumber(int versionNumber) {
            mVersionNumber = versionNumber;
            return this;
        }

        public RecordedProgram build() {
            return new RecordedProgram(mId, mInputId, mChannelId, mTitle, mSeasonNumber,
                    mSeasonTitle, mEpisodeNumber, mEpisodeTitle, mStartTimeUtcMillis,
                    mEndTimeUtcMillis, mBroadcastGenres, mCanonicalGenres, mShortDescription,
                    mLongDescription, mVideoWidth, mVideoHeight, mAudioLanguage, mContentRating,
                    mPosterArt, mThumbnail, mSearchable, mDataUri, mDataBytes, mDurationMillis,
                    mExpireTimeUtcMillis, mInternalProviderData, mInternalProviderFlag1,
                    mInternalProviderFlag2, mInternalProviderFlag3, mInternalProviderFlag4,
                    mVersionNumber);
        }
    }

    public static Builder builder() { return new Builder(); }

    public static Builder buildFrom(RecordedProgram orig) {
        return builder()
                .setId(orig.getId())
                .setInputId(orig.getInputId())
                .setChannelId(orig.getChannelId())
                .setTitle(orig.getTitle())
                .setSeasonNumber(orig.getSeasonNumber())
                .setSeasonTitle(orig.getSeasonTitle())
                .setEpisodeNumber(orig.getEpisodeNumber())
                .setEpisodeTitle(orig.getEpisodeTitle())
                .setStartTimeUtcMillis(orig.getStartTimeUtcMillis())
                .setEndTimeUtcMillis(orig.getEndTimeUtcMillis())
                .setBroadcastGenres(orig.getBroadcastGenres())
                .setCanonicalGenres(orig.getCanonicalGenres())
                .setShortDescription(orig.getShortDescription())
                .setLongDescription(orig.getLongDescription())
                .setVideoWidth(orig.getVideoWidth())
                .setVideoHeight(orig.getVideoHeight())
                .setAudioLanguage(orig.getAudioLanguage())
                .setContentRating(orig.getContentRating())
                .setPosterArt(orig.getPosterArt())
                .setThumbnail(orig.getThumbnail())
                .setSearchable(orig.isSearchable())
                .setInternalProviderData(orig.getInternalProviderData())
                .setInternalProviderFlag1(orig.getInternalProviderFlag1())
                .setInternalProviderFlag2(orig.getInternalProviderFlag2())
                .setInternalProviderFlag3(orig.getInternalProviderFlag3())
                .setInternalProviderFlag4(orig.getInternalProviderFlag4())
                .setVersionNumber(orig.getVersionNumber());
    }

    public static final Comparator<RecordedProgram> START_TIME_THEN_ID_COMPARATOR
            = new Comparator<RecordedProgram>() {
        @Override
        public int compare(RecordedProgram lhs, RecordedProgram rhs) {
            int res = Long.compare(lhs.getStartTimeUtcMillis(), rhs.getStartTimeUtcMillis());
            if (res != 0) {
                return res;
            }
            return Long.compare(lhs.mId, rhs.mId);
        }
    };

    private final long mId;
    private final String mInputId;
    private final long mChannelId;
    private final String mTitle;
    private final String mSeasonNumber;
    private final String mSeasonTitle;
    private final String mEpisodeNumber;
    private final String mEpisodeTitle;
    private final long mStartTimeUtcMillis;
    private final long mEndTimeUtcMillis;
    private final String[] mBroadcastGenres;
    private final String[] mCanonicalGenres;
    private final String mShortDescription;
    private final String mLongDescription;
    private final int mVideoWidth;
    private final int mVideoHeight;
    private final String mAudioLanguage;
    private final String mContentRating;
    private final Uri mPosterArt;
    private final Uri mThumbnail;
    private final boolean mSearchable;
    private final Uri mDataUri;
    private final long mDataBytes;
    private final long mDurationMillis;
    private final long mExpireTimeUtcMillis;
    private final byte[] mInternalProviderData;
    private final int mInternalProviderFlag1;
    private final int mInternalProviderFlag2;
    private final int mInternalProviderFlag3;
    private final int mInternalProviderFlag4;
    private final int mVersionNumber;

    private RecordedProgram(long id, String inputId, long channelId, String title,
            String seasonNumber, String seasonTitle, String episodeNumber, String episodeTitle,
            long startTimeUtcMillis, long endTimeUtcMillis, String[] broadcastGenres,
            String[] canonicalGenres, String shortDescription, String longDescription,
            int videoWidth, int videoHeight, String audioLanguage, String contentRating,
            Uri posterArt, Uri thumbnail, boolean searchable, Uri dataUri, long dataBytes,
            long durationMillis, long expireTimeUtcMillis, byte[] internalProviderData,
            int internalProviderFlag1, int internalProviderFlag2, int internalProviderFlag3,
            int internalProviderFlag4, int versionNumber) {
        mId = id;
        mInputId = inputId;
        mChannelId = channelId;
        mTitle = title;
        mSeasonNumber = seasonNumber;
        mSeasonTitle = seasonTitle;
        mEpisodeNumber = episodeNumber;
        mEpisodeTitle = episodeTitle;
        mStartTimeUtcMillis = startTimeUtcMillis;
        mEndTimeUtcMillis = endTimeUtcMillis;
        mBroadcastGenres = broadcastGenres;
        mCanonicalGenres = canonicalGenres;
        mShortDescription = shortDescription;
        mLongDescription = longDescription;
        mVideoWidth = videoWidth;
        mVideoHeight = videoHeight;

        mAudioLanguage = audioLanguage;
        mContentRating = contentRating;
        mPosterArt = posterArt;
        mThumbnail = thumbnail;
        mSearchable = searchable;
        mDataUri = dataUri;
        mDataBytes = dataBytes;
        mDurationMillis = durationMillis;
        mExpireTimeUtcMillis = expireTimeUtcMillis;
        mInternalProviderData = internalProviderData;
        mInternalProviderFlag1 = internalProviderFlag1;
        mInternalProviderFlag2 = internalProviderFlag2;
        mInternalProviderFlag3 = internalProviderFlag3;
        mInternalProviderFlag4 = internalProviderFlag4;
        mVersionNumber = versionNumber;
    }

    public String getAudioLanguage() {
        return mAudioLanguage;
    }

    public String[] getBroadcastGenres() {
        return mBroadcastGenres;
    }

    public String[] getCanonicalGenres() {
        return mCanonicalGenres;
    }

    public long getChannelId() {
        return mChannelId;
    }

    public String getContentRating() {
        return mContentRating;
    }

    public Uri getDataUri() {
        return mDataUri;
    }

    public long getDataBytes() {
        return mDataBytes;
    }

    public long getDurationMillis() {
        return mDurationMillis;
    }

    public long getEndTimeUtcMillis() {
        return mEndTimeUtcMillis;
    }

    public String getEpisodeNumber() {
        return mEpisodeNumber;
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

    public long getExpireTimeUtcMillis() {
        return mExpireTimeUtcMillis;
    }

    public long getId() {
        return mId;
    }

    public String getInputId() {
        return mInputId;
    }

    public byte[] getInternalProviderData() {
        return mInternalProviderData;
    }

    public int getInternalProviderFlag1() {
        return mInternalProviderFlag1;
    }

    public int getInternalProviderFlag2() {
        return mInternalProviderFlag2;
    }

    public int getInternalProviderFlag3() {
        return mInternalProviderFlag3;
    }

    public int getInternalProviderFlag4() {
        return mInternalProviderFlag4;
    }

    public String getLongDescription() {
        return mLongDescription;
    }

    public Uri getPosterArt() {
        return mPosterArt;
    }

    public boolean isSearchable() {
        return mSearchable;
    }

    public String getSeasonNumber() {
        return mSeasonNumber;
    }

    public String getSeasonTitle() {
        return mSeasonTitle;
    }

    public String getShortDescription() {
        return mShortDescription;
    }

    public long getStartTimeUtcMillis() {
        return mStartTimeUtcMillis;
    }

    public Uri getThumbnail() {
        return mThumbnail;
    }

    public String getTitle() {
        return mTitle;
    }

    public Uri getUri() {
        return ContentUris.withAppendedId(RecordedPrograms.CONTENT_URI, mId);
    }

    public int getVersionNumber() {
        return mVersionNumber;
    }

    public int getVideoHeight() {
        return mVideoHeight;
    }

    public int getVideoWidth() {
        return mVideoWidth;
    }

    /**
     * Compares everything except {@link #getInternalProviderData()}
     */
    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        RecordedProgram that = (RecordedProgram) o;
        return Objects.equals(mId, that.mId) &&
                Objects.equals(mChannelId, that.mChannelId) &&
                Objects.equals(mSeasonNumber, that.mSeasonNumber) &&
                Objects.equals(mSeasonTitle, that.mSeasonTitle) &&
                Objects.equals(mEpisodeNumber, that.mEpisodeNumber) &&
                Objects.equals(mStartTimeUtcMillis, that.mStartTimeUtcMillis) &&
                Objects.equals(mEndTimeUtcMillis, that.mEndTimeUtcMillis) &&
                Objects.equals(mVideoWidth, that.mVideoWidth) &&
                Objects.equals(mVideoHeight, that.mVideoHeight) &&
                Objects.equals(mSearchable, that.mSearchable) &&
                Objects.equals(mDataBytes, that.mDataBytes) &&
                Objects.equals(mDurationMillis, that.mDurationMillis) &&
                Objects.equals(mExpireTimeUtcMillis, that.mExpireTimeUtcMillis) &&
                Objects.equals(mInternalProviderFlag1, that.mInternalProviderFlag1) &&
                Objects.equals(mInternalProviderFlag2, that.mInternalProviderFlag2) &&
                Objects.equals(mInternalProviderFlag3, that.mInternalProviderFlag3) &&
                Objects.equals(mInternalProviderFlag4, that.mInternalProviderFlag4) &&
                Objects.equals(mVersionNumber, that.mVersionNumber) &&
                Objects.equals(mTitle, that.mTitle) &&
                Objects.equals(mEpisodeTitle, that.mEpisodeTitle) &&
                Arrays.equals(mBroadcastGenres, that.mBroadcastGenres) &&
                Arrays.equals(mCanonicalGenres, that.mCanonicalGenres) &&
                Objects.equals(mShortDescription, that.mShortDescription) &&
                Objects.equals(mLongDescription, that.mLongDescription) &&
                Objects.equals(mAudioLanguage, that.mAudioLanguage) &&
                Objects.equals(mContentRating, that.mContentRating) &&
                Objects.equals(mPosterArt, that.mPosterArt) &&
                Objects.equals(mThumbnail, that.mThumbnail);
    }

    /**
     * Hashes based on the ID.
     */
    @Override
    public int hashCode() {
        return Objects.hash(mId);
    }

    @Override
    public String toString() {
        return "RecordedProgram"
                + "[" +  mId +
                "]{ mInputId=" + mInputId +
                ", mChannelId='" + mChannelId + '\'' +
                ", mTitle='" + mTitle + '\'' +
                ", mEpisodeNumber=" + mEpisodeNumber +
                ", mEpisodeTitle='" + mEpisodeTitle + '\'' +
                ", mStartTimeUtcMillis=" + mStartTimeUtcMillis +
                ", mEndTimeUtcMillis=" + mEndTimeUtcMillis +
                ", mBroadcastGenres=" +
                        (mBroadcastGenres != null ? Arrays.toString(mBroadcastGenres) : "null") +
                ", mCanonicalGenres=" +
                        (mCanonicalGenres != null ? Arrays.toString(mCanonicalGenres) : "null") +
                ", mShortDescription='" + mShortDescription + '\'' +
                ", mLongDescription='" + mLongDescription + '\'' +
                ", mVideoHeight=" + mVideoHeight +
                ", mVideoWidth=" + mVideoWidth +
                ", mAudioLanguage='" + mAudioLanguage + '\'' +
                ", mContentRating='" + mContentRating + '\'' +
                ", mPosterArt=" + mPosterArt +
                ", mThumbnail=" + mThumbnail +
                ", mSearchable=" + mSearchable +
                ", mDataUri=" + mDataUri +
                ", mDataBytes=" + mDataBytes +
                ", mDurationMillis=" + mDurationMillis +
                ", mExpireTimeUtcMillis=" + mExpireTimeUtcMillis +
                ", mInternalProviderData.length=" +
                        (mInternalProviderData == null ? "null" : mInternalProviderData.length) +
                ", mInternalProviderFlag1=" + mInternalProviderFlag1 +
                ", mInternalProviderFlag2=" + mInternalProviderFlag2 +
                ", mInternalProviderFlag3=" + mInternalProviderFlag3 +
                ", mInternalProviderFlag4=" + mInternalProviderFlag4 +
                ", mSeasonNumber=" + mSeasonNumber +
                ", mSeasonTitle=" + mSeasonTitle +
                ", mVersionNumber=" + mVersionNumber +
                '}';
    }

    @Nullable
    private static String safeToString(@Nullable Object o) {
        return o == null ? null : o.toString();
    }

    @Nullable
    private static String safeEncode(@Nullable String[] genres) {
        return genres == null ? null : TvContract.Programs.Genres.encode(genres);
    }
}
