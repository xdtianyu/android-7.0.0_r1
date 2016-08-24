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

package com.android.bluetooth.avrcp;

import android.util.Log;

import com.android.bluetooth.Utils;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

/**
 * Provides Bluetooth AVRCP Controller profile, as a service in the Bluetooth application.
 * TODO(sanketa): Rip out this feature as this is part of 1.6.
 * @hide
 */
public class NowPlaying {
    private static final boolean DBG = true;
    private static final String TAG = "NowPlaying";

    RemoteDevice mDevice;
    private TrackInfo mCurrTrack;

    private ArrayList<TrackInfo> mNowPlayingList;

    public NowPlaying(RemoteDevice mRemoteDevice) {
        mDevice = mRemoteDevice;
        mNowPlayingList = new ArrayList<TrackInfo>();
        mCurrTrack = null;
    }

    public void cleanup() {
        mDevice = null;
        if(mNowPlayingList != null) {
            mNowPlayingList.clear();
        }
        mCurrTrack = null;
    }

    public RemoteDevice getDeviceRecords() {
        return mDevice;
    }

    public void addTrack (TrackInfo mTrack) {
        if(mNowPlayingList != null) {
            mNowPlayingList.add(mTrack);
        }
    }

    public void setCurrTrack (TrackInfo mTrack) {
        mCurrTrack = mTrack;
    }

    public TrackInfo getCurrentTrack() {
        return mCurrTrack;
    }

    public void updateCurrentTrack(TrackInfo mTrack) {
        mCurrTrack.mAlbumTitle = mTrack.mAlbumTitle;
        mCurrTrack.mArtistName = mTrack.mArtistName;
        mCurrTrack.mGenre = mTrack.mGenre;
        mCurrTrack.mTotalTracks = mTrack.mTotalTracks;
        mCurrTrack.mTrackLen = mTrack.mTrackLen;
        mCurrTrack.mTrackTitle = mTrack.mTrackTitle;
        mCurrTrack.mTrackNum = mTrack.mTrackNum;
    }

    public TrackInfo getTrackFromId(int mTrackId) {
        if(mTrackId == 0)
            return getCurrentTrack();
        else {
            for(TrackInfo mTrackInfo: mNowPlayingList) {
                if(mTrackInfo.mItemUid == mTrackId)
                    return mTrackInfo;
            }
            return null;
        }
    }
}
