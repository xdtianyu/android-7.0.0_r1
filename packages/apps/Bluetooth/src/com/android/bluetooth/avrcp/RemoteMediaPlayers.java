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
public class RemoteMediaPlayers {
    private static final boolean DBG = true;
    private static final String TAG = "RemoteMediaPlayers";

    RemoteDevice mDevice;
    private PlayerInfo mAddressedPlayer;
    private PlayerInfo mBrowsedPlayer;
    ArrayList<PlayerInfo> mMediaPlayerList;

    public RemoteMediaPlayers (RemoteDevice mRemoteDevice) {
        mDevice = mRemoteDevice;
        mAddressedPlayer = null;
        mBrowsedPlayer = null;
        mMediaPlayerList = new ArrayList<PlayerInfo>();
    }

    public void cleanup() {
        mDevice = null;
        mAddressedPlayer = null;
        mBrowsedPlayer = null;
        if(mMediaPlayerList != null)
            mMediaPlayerList.clear();
    }
    /*
     * add a Player
     */
    public void addPlayer (PlayerInfo mPlayer) {
        if(mMediaPlayerList != null)
            mMediaPlayerList.add(mPlayer);
    }
    /*
     * add players and Set AddressedPlayer and BrowsePlayer
     */
    public void setAddressedPlayer(PlayerInfo mPlayer) {
        mAddressedPlayer = mPlayer;
    }

    public void setBrowsedPlayer(PlayerInfo mPlayer) {
        mBrowsedPlayer = mPlayer;
    }
    /*
     * Returns the currently addressed, browsed player
     */
    public PlayerInfo getAddressedPlayer() {
        return mAddressedPlayer;
    }

    /*
     * getPlayStatus of addressed player
     */
    public byte getPlayStatus() {
        if(getAddressedPlayer() != null)
            return getAddressedPlayer().mPlayStatus;
        else
            return AvrcpControllerConstants.PLAY_STATUS_STOPPED;
    }

    /*
     * getPlayStatus of addressed player
     */
    public long getPlayPosition() {
        if(getAddressedPlayer() != null)
            return getAddressedPlayer().mPlayTime;
        else
            return AvrcpControllerConstants.PLAYING_TIME_INVALID;
    }

}
