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

package com.android.usbtuner.data;

import android.support.annotation.NonNull;
import android.util.Log;

import com.android.usbtuner.data.Channel;
import com.android.usbtuner.data.Channel.TunerChannelProto;
import com.android.usbtuner.data.PsiData.PmtItem;
import com.android.usbtuner.data.PsipData.TvTracksInterface;
import com.android.usbtuner.data.PsipData.VctItem;
import com.android.usbtuner.data.Track.AtscAudioTrack;
import com.android.usbtuner.data.Track.AtscCaptionTrack;
import com.android.usbtuner.util.Ints;
import com.android.usbtuner.util.StringUtils;
import com.google.protobuf.nano.MessageNano;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * A class that represents a single channel accessible through a tuner.
 */
public class TunerChannel implements Comparable<TunerChannel>, TvTracksInterface {
    private static final String TAG = "TunerChannel";

    // See ATSC Code Points Registry.
    private static final String[] ATSC_SERVICE_TYPE_NAMES = new String[] {
            "ATSC Reserved",
            "Analog television channels",
            "ATSC_digital_television",
            "ATSC_audio",
            "ATSC_data_only_service",
            "Software Download",
            "Unassociated/Small Screen Service",
            "Parameterized Service",
            "ATSC NRT Service",
            "Extended Parameterized Service" };
    private static final String ATSC_SERVICE_TYPE_NAME_RESERVED =
            ATSC_SERVICE_TYPE_NAMES[Channel.SERVICE_TYPE_ATSC_RESERVED];
    private static final String ATSC_SERVICE_TYPE_NAME_DIGITAL_TELEVISION =
            ATSC_SERVICE_TYPE_NAMES[Channel.SERVICE_TYPE_ATSC_DIGITAL_TELEVISION];

    public static final int INVALID_FREQUENCY = -1;

    // According to RFC4259, The number of available PIDs ranges from 0 to 8191.
    public static final int INVALID_PID = -1;

    // According to ISO13818-1, Mpeg2 StreamType has a range from 0x00 to 0xff.
    public static final int INVALID_STREAMTYPE = -1;

    private final TunerChannelProto mProto;

    private TunerChannel(VctItem channel, int programNumber, List<PmtItem> pmtItems, int type) {
        mProto = new TunerChannelProto();
        if (channel == null) {
            mProto.shortName = "";
            mProto.tsid = 0;
            mProto.programNumber = programNumber;
            mProto.virtualMajor = 0;
            mProto.virtualMinor = 0;
        } else {
            mProto.shortName = channel.getShortName();
            if (channel.getLongName() != null) {
                mProto.longName = channel.getLongName();
            }
            mProto.tsid = channel.getChannelTsid();
            mProto.programNumber = channel.getProgramNumber();
            mProto.virtualMajor = channel.getMajorChannelNumber();
            mProto.virtualMinor = channel.getMinorChannelNumber();
            if (channel.getDescription() != null) {
                mProto.description = channel.getDescription();
            }
            mProto.serviceType = channel.getServiceType();
        }
        mProto.type = type;
        mProto.channelId = -1L;
        mProto.frequency = INVALID_FREQUENCY;
        mProto.videoPid = INVALID_PID;
        mProto.videoStreamType = INVALID_STREAMTYPE;
        List<Integer> audioPids = new ArrayList<>();
        List<Integer> audioStreamTypes = new ArrayList<>();
        for (PsiData.PmtItem pmt : pmtItems) {
            switch (pmt.getStreamType()) {
                // MPEG ES stream video types
                case Channel.MPEG1:
                case Channel.MPEG2:
                case Channel.H263:
                case Channel.H264:
                case Channel.H265:
                    mProto.videoPid = pmt.getEsPid();
                    mProto.videoStreamType = pmt.getStreamType();
                    break;

                // MPEG ES stream audio types
                case Channel.MPEG1AUDIO:
                case Channel.MPEG2AUDIO:
                case Channel.MPEG2AACAUDIO:
                case Channel.MPEG4LATMAACAUDIO:
                case Channel.A52AC3AUDIO:
                case Channel.EAC3AUDIO:
                    audioPids.add(pmt.getEsPid());
                    audioStreamTypes.add(pmt.getStreamType());
                    break;

                // Non MPEG ES stream types
                case 0x100: // PmtItem.ES_PID_PCR:
                    mProto.pcrPid = pmt.getEsPid();
                    break;
            }
        }
        mProto.audioPids = Ints.toArray(audioPids);
        mProto.audioStreamTypes = Ints.toArray(audioStreamTypes);
        mProto.audioTrackIndex = (audioPids.size() > 0) ? 0 : -1;
    }

    public TunerChannel(VctItem channel, List<PmtItem> pmtItems) {
        this(channel, 0, pmtItems, Channel.TYPE_TUNER);
    }

    public TunerChannel(int programNumber, List<PmtItem> pmtItems) {
        this(null, programNumber, pmtItems, Channel.TYPE_TUNER);
    }

    private TunerChannel(TunerChannelProto tunerChannelProto) {
        mProto = tunerChannelProto;
    }

    public static TunerChannel forFile(VctItem channel, List<PmtItem> pmtItems) {
        return new TunerChannel(channel, 0, pmtItems, Channel.TYPE_FILE);
    }

    public String getName() {
        return (mProto.longName.isEmpty()) ? mProto.shortName : mProto.longName;
    }

    public String getShortName() {
        return mProto.shortName;
    }

    public int getProgramNumber() {
        return mProto.programNumber;
    }

    public int getServiceType() {
        return mProto.serviceType;
    }

    public String getServiceTypeName() {
        int serviceType = mProto.serviceType;
        if (serviceType >= 0 && serviceType < ATSC_SERVICE_TYPE_NAMES.length) {
            return ATSC_SERVICE_TYPE_NAMES[serviceType];
        }
        return ATSC_SERVICE_TYPE_NAME_RESERVED;
    }

    public int getVirtualMajor() {
        return mProto.virtualMajor;
    }

    public int getVirtualMinor() {
        return mProto.virtualMinor;
    }

    public int getFrequency() {
        return mProto.frequency;
    }

    public String getModulation() {
        return mProto.modulation;
    }

    public int getTsid() {
        return mProto.tsid;
    }

    public int getVideoPid() {
        return mProto.videoPid;
    }

    public void setVideoPid(int videoPid) {
        mProto.videoPid = videoPid;
    }

    public int getVideoStreamType() {
        return mProto.videoStreamType;
    }

    public int getAudioPid() {
        if (mProto.audioTrackIndex == -1) {
            return INVALID_PID;
        }
        return mProto.audioPids[mProto.audioTrackIndex];
    }

    public int getAudioStreamType() {
        if (mProto.audioTrackIndex == -1) {
            return INVALID_STREAMTYPE;
        }
        return mProto.audioStreamTypes[mProto.audioTrackIndex];
    }

    public List<Integer> getAudioPids() {
        return Ints.asList(mProto.audioPids);
    }

    public void setAudioPids(List<Integer> audioPids) {
        mProto.audioPids = Ints.toArray(audioPids);
    }

    public List<Integer> getAudioStreamTypes() {
        return Ints.asList(mProto.audioStreamTypes);
    }

    public void setAudioStreamTypes(List<Integer> audioStreamTypes) {
        mProto.audioStreamTypes = Ints.toArray(audioStreamTypes);
    }

    public int getPcrPid() {
        return mProto.pcrPid;
    }

    public int getType() {
        return mProto.type;
    }

    public void setFilepath(String filepath) {
        mProto.filepath = filepath;
    }

    public String getFilepath() {
        return mProto.filepath;
    }

    public void setFrequency(int frequency) {
        mProto.frequency = frequency;
    }

    public void setModulation(String modulation) {
        mProto.modulation = modulation;
    }

    public boolean hasVideo() {
        return mProto.videoPid != INVALID_PID;
    }

    public boolean hasAudio() {
        return getAudioPid() != INVALID_PID;
    }

    public long getChannelId() {
        return mProto.channelId;
    }

    public void setChannelId(long channelId) {
        mProto.channelId = channelId;
    }

    public String getDisplayNumber() {
        if (mProto.virtualMajor != 0 && mProto.virtualMinor != 0) {
            return String.format("%d-%d", mProto.virtualMajor, mProto.virtualMinor);
        } else if (mProto.virtualMajor != 0) {
            return Integer.toString(mProto.virtualMajor);
        } else {
            return Integer.toString(mProto.programNumber);
        }
    }

    public String getDescription() {
        return mProto.description;
    }

    @Override
    public void setHasCaptionTrack() {
        mProto.hasCaptionTrack = true;
    }

    @Override
    public boolean hasCaptionTrack() {
        return mProto.hasCaptionTrack;
    }

    @Override
    public List<AtscAudioTrack> getAudioTracks() {
        return Collections.unmodifiableList(Arrays.asList(mProto.audioTracks));
    }

    public void setAudioTracks(List<AtscAudioTrack> audioTracks) {
        mProto.audioTracks = audioTracks.toArray(new AtscAudioTrack[audioTracks.size()]);
    }

    @Override
    public List<AtscCaptionTrack> getCaptionTracks() {
        return Collections.unmodifiableList(Arrays.asList(mProto.captionTracks));
    }

    public void setCaptionTracks(List<AtscCaptionTrack> captionTracks) {
        mProto.captionTracks = captionTracks.toArray(new AtscCaptionTrack[captionTracks.size()]);
    }

    public void selectAudioTrack(int index) {
        if (0 <= index && index < mProto.audioPids.length) {
            mProto.audioTrackIndex = index;
        } else {
            mProto.audioTrackIndex = -1;
        }
    }

    @Override
    public String toString() {
        switch (mProto.type) {
            case Channel.TYPE_FILE:
                return String.format("{%d-%d %s} Filepath: %s, ProgramNumber %d",
                        mProto.virtualMajor, mProto.virtualMinor, mProto.shortName,
                        mProto.filepath, mProto.programNumber);
            //case Channel.TYPE_TUNER:
            default:
                return String.format("{%d-%d %s} Frequency: %d, ProgramNumber %d",
                        mProto.virtualMajor, mProto.virtualMinor, mProto.shortName,
                        mProto.frequency, mProto.programNumber);
        }
    }

    @Override
    public int compareTo(@NonNull TunerChannel channel) {
        // In the same frequency, the program number acts as the sub-channel number.
        int ret = getFrequency() - channel.getFrequency();
        if (ret != 0) {
            return ret;
        }
        ret = getProgramNumber() - channel.getProgramNumber();
        if (ret != 0) {
            return ret;
        }

        // For FileDataSource, file paths should be compared.
        return StringUtils.compare(getFilepath(), channel.getFilepath());
    }

    // Serialization
    public byte[] toByteArray() {
        return MessageNano.toByteArray(mProto);
    }

    public static TunerChannel parseFrom(byte[] data) {
        if (data == null) {
            return null;
        }
        try {
            return new TunerChannel(TunerChannelProto.parseFrom(data));
        } catch (IOException e) {
            Log.e(TAG, "Could not parse from byte array", e);
            return null;
        }
    }
}
