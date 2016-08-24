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

package com.android.usbtuner.tvinput;

import android.util.Log;
import android.util.SparseArray;
import android.util.SparseBooleanArray;

import com.android.usbtuner.TunerHal;
import com.android.usbtuner.data.PsiData.PatItem;
import com.android.usbtuner.data.PsiData.PmtItem;
import com.android.usbtuner.data.PsipData.EitItem;
import com.android.usbtuner.data.PsipData.VctItem;
import com.android.usbtuner.data.Track.AtscAudioTrack;
import com.android.usbtuner.data.Track.AtscCaptionTrack;
import com.android.usbtuner.data.TunerChannel;
import com.android.usbtuner.ts.TsParser;
import com.android.usbtuner.ts.TsParser.TsOutputListener;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Detects channels and programs that are emerged or changed while parsing ATSC PSIP information.
 */
public class EventDetector {
    private static final String TAG = "EventDetector";
    private static final boolean DEBUG = false;

    private final TunerHal mTunerHal;

    private TsParser mTsParser;
    private final Set<Integer> mPidSet = new HashSet<>();

    // To prevent channel duplication
    private final Set<Integer> mVctProgramNumberSet = new HashSet<>();
    private final SparseArray<TunerChannel> mChannelMap = new SparseArray<>();
    private final SparseBooleanArray mVctCaptionTracksFound = new SparseBooleanArray();
    private final SparseBooleanArray mEitCaptionTracksFound = new SparseBooleanArray();
    private EventListener mEventListener;
    private int mFrequency;
    private String mModulation;

    private TsOutputListener mTsOutputListener = new TsOutputListener() {
        @Override
        public void onPatDetected(List<PatItem> items) {
            for (PatItem i : items) {
                mTunerHal.addPidFilter(i.getPmtPid(), TunerHal.FILTER_TYPE_OTHER);
            }
        }

        @Override
        public void onEitPidDetected(int pid) {
            startListening(pid);
        }

        @Override
        public void onEitItemParsed(VctItem channel, List<EitItem> items) {
            TunerChannel tunerChannel = mChannelMap.get(channel.getProgramNumber());
            if (DEBUG) {
                Log.d(TAG, "onEitItemParsed tunerChannel:" + tunerChannel + " "
                        + channel.getProgramNumber());
            }
            int channelSourceId = channel.getSourceId();

            // Source id 0 is useful for cases where a cable operator wishes to define a channel for
            // which no EPG data is currently available.
            // We don't handle such a case.
            if (channelSourceId == 0) {
                return;
            }

            // If at least a one caption track have been found in EIT items for the given channel,
            // we starts to interpret the zero tracks as a clearance of the caption tracks.
            boolean captionTracksFound = mEitCaptionTracksFound.get(channelSourceId);
            for (EitItem item : items) {
                if (captionTracksFound) {
                    break;
                }
                List<AtscCaptionTrack> captionTracks = item.getCaptionTracks();
                if (captionTracks != null && !captionTracks.isEmpty()) {
                    captionTracksFound = true;
                }
            }
            mEitCaptionTracksFound.put(channelSourceId, captionTracksFound);
            if (captionTracksFound) {
                for (EitItem item : items) {
                    item.setHasCaptionTrack();
                }
            }
            if (tunerChannel != null && mEventListener != null) {
                mEventListener.onEventDetected(tunerChannel, items);
            }
        }

        @Override
        public void onEttPidDetected(int pid) {
            startListening(pid);
        }

        @Override
        public void onVctItemParsed(VctItem channel, List<PmtItem> pmtItems) {
            if (DEBUG) {
                Log.d(TAG, "onVctItemParsed VCT " + channel);
                Log.d(TAG, "                PMT " + pmtItems);
            }

            // Merges the audio and caption tracks located in PMT items into the tracks of the given
            // tuner channel.
            TunerChannel tunerChannel = new TunerChannel(channel, pmtItems);
            List<AtscAudioTrack> audioTracks = new ArrayList<>();
            List<AtscCaptionTrack> captionTracks = new ArrayList<>();
            for (PmtItem pmtItem : pmtItems) {
                if (pmtItem.getAudioTracks() != null) {
                    audioTracks.addAll(pmtItem.getAudioTracks());
                }
                if (pmtItem.getCaptionTracks() != null) {
                    captionTracks.addAll(pmtItem.getCaptionTracks());
                }
            }
            int channelProgramNumber = channel.getProgramNumber();

            // If at least a one caption track have been found in VCT items for the given channel,
            // we starts to interpret the zero tracks as a clearance of the caption tracks.
            boolean captionTracksFound = mVctCaptionTracksFound.get(channelProgramNumber)
                    || !captionTracks.isEmpty();
            mVctCaptionTracksFound.put(channelProgramNumber, captionTracksFound);
            if (captionTracksFound) {
                tunerChannel.setHasCaptionTrack();
            }
            tunerChannel.setAudioTracks(audioTracks);
            tunerChannel.setCaptionTracks(captionTracks);
            tunerChannel.setFrequency(mFrequency);
            tunerChannel.setModulation(mModulation);
            mChannelMap.put(tunerChannel.getProgramNumber(), tunerChannel);
            boolean found = mVctProgramNumberSet.contains(channelProgramNumber);
            if (!found) {
                mVctProgramNumberSet.add(channelProgramNumber);
            }
            if (mEventListener != null) {
                mEventListener.onChannelDetected(tunerChannel, !found);
            }
        }
    };

    /**
     * Listener for detecting ATSC TV channels and receiving EPG data.
     */
    public interface EventListener {

        /**
         * Fired when new information of an ATSC TV channel arrived.
         *
         * @param channel an ATSC TV channel
         * @param channelArrivedAtFirstTime tells whether this channel arrived at first time
         */
        void onChannelDetected(TunerChannel channel, boolean channelArrivedAtFirstTime);

        /**
         * Fired when new program events of an ATSC TV channel arrived.
         *
         * @param channel an ATSC TV channel
         * @param items a list of EIT items that were received
         */
        void onEventDetected(TunerChannel channel, List<EitItem> items);
    }

    public EventDetector(TunerHal usbTunerInteface, EventListener listener) {
        mTunerHal = usbTunerInteface;
        mEventListener = listener;
    }

    private void reset() {
        mTsParser = new TsParser(mTsOutputListener); // TODO: Use TsParser.reset()
        mPidSet.clear();
        mVctProgramNumberSet.clear();
        mVctCaptionTracksFound.clear();
        mEitCaptionTracksFound.clear();
        mChannelMap.clear();
    }

    public void startDetecting(int frequency, String modulation) {
        reset();
        mFrequency = frequency;
        mModulation = modulation;
    }

    private void startListening(int pid) {
        if (mPidSet.contains(pid)) {
            return;
        }
        mPidSet.add(pid);
        mTunerHal.addPidFilter(pid, TunerHal.FILTER_TYPE_OTHER);
    }

    public void feedTSStream(byte[] data, int startOffset, int length) {
        if (mPidSet.isEmpty()) {
            startListening(TsParser.ATSC_SI_BASE_PID);
        }
        if (mTsParser != null) {
            mTsParser.feedTSData(data, startOffset, length);
        }
    }

    public List<TunerChannel> getIncompleteChannels() {
        return mTsParser.getIncompleteChannels();
    }
}
