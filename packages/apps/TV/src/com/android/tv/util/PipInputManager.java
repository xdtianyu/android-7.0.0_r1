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

package com.android.tv.util;

import android.content.Context;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputManager.TvInputCallback;
import android.util.ArraySet;
import android.util.Log;

import com.android.tv.ChannelTuner;
import com.android.tv.R;
import com.android.tv.data.Channel;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * A class that manages inputs for PIP. All tuner inputs are represented to one tuner input for PIP.
 * Hidden inputs should not be visible to the users.
 */
public class PipInputManager {
    private static final String TAG = "PipInputManager";

    // Tuner inputs aren't distinguished each other in PipInput. They are handled as one input.
    // Therefore, we define a fake input id for the unified input.
    private static final String TUNER_INPUT_ID = "tuner_input_id";

    private final Context mContext;
    private final TvInputManagerHelper mInputManager;
    private final ChannelTuner mChannelTuner;
    private boolean mStarted;
    private final Map<String, PipInput> mPipInputMap = new HashMap<>();  // inputId -> PipInput
    private final Set<Listener> mListeners = new ArraySet<>();

    private final TvInputCallback mTvInputCallback = new TvInputCallback() {
        @Override
        public void onInputAdded(String inputId) {
            TvInputInfo input = mInputManager.getTvInputInfo(inputId);
            if (input.isPassthroughInput()) {
                boolean available = mInputManager.getInputState(input)
                        == TvInputManager.INPUT_STATE_CONNECTED;
                mPipInputMap.put(inputId, new PipInput(inputId, available));
            } else if (!mPipInputMap.containsKey(TUNER_INPUT_ID)) {
                boolean available = mChannelTuner.getBrowsableChannelCount() != 0;
                mPipInputMap.put(TUNER_INPUT_ID, new PipInput(TUNER_INPUT_ID, available));
            } else {
                return;
            }
            for (Listener l : mListeners) {
                l.onPipInputListUpdated();
            }
        }

        @Override
        public void onInputRemoved(String inputId) {
            PipInput pipInput = mPipInputMap.remove(inputId);
            if (pipInput == null) {
                if (!mPipInputMap.containsKey(TUNER_INPUT_ID)) {
                    Log.w(TAG, "A TV input (" + inputId + ") isn't tracked in PipInputManager");
                    return;
                }
                if (mInputManager.getTunerTvInputSize() > 0) {
                    return;
                }
                mPipInputMap.remove(TUNER_INPUT_ID);
            }
            for (Listener l : mListeners) {
                l.onPipInputListUpdated();
            }
        }

        @Override
        public void onInputStateChanged(String inputId, int state) {
            PipInput pipInput = mPipInputMap.get(inputId);
            if (pipInput == null) {
                // For tuner input, state change is handled in mChannelTunerListener.
                return;
            }
            pipInput.updateAvailability();
        }
    };

    private final ChannelTuner.Listener mChannelTunerListener = new ChannelTuner.Listener() {
        @Override
        public void onLoadFinished() { }

        @Override
        public void onCurrentChannelUnavailable(Channel channel) { }

        @Override
        public void onBrowsableChannelListChanged() {
            PipInput tunerInput = mPipInputMap.get(TUNER_INPUT_ID);
            if (tunerInput == null) {
                return;
            }
            tunerInput.updateAvailability();
        }

        @Override
        public void onChannelChanged(Channel previousChannel, Channel currentChannel) {
            if (previousChannel != null && currentChannel != null
                    && !previousChannel.isPassthrough() && !currentChannel.isPassthrough()) {
                // Channel change between channels for tuner inputs.
                return;
            }
            PipInput previousMainInput = getPipInput(previousChannel);
            if (previousMainInput != null) {
                previousMainInput.updateAvailability();
            }
            PipInput currentMainInput = getPipInput(currentChannel);
            if (currentMainInput != null) {
                currentMainInput.updateAvailability();
            }
        }
    };

    public PipInputManager(Context context, TvInputManagerHelper inputManager,
            ChannelTuner channelTuner) {
        mContext = context;
        mInputManager = inputManager;
        mChannelTuner = channelTuner;
    }

    /**
     * Starts {@link PipInputManager}.
     */
    public void start() {
        if (mStarted) {
            return;
        }
        mInputManager.addCallback(mTvInputCallback);
        mChannelTuner.addListener(mChannelTunerListener);
        initializePipInputList();
    }

    /**
     * Stops {@link PipInputManager}.
     */
    public void stop() {
        if (!mStarted) {
            return;
        }
        mInputManager.removeCallback(mTvInputCallback);
        mChannelTuner.removeListener(mChannelTunerListener);
        mPipInputMap.clear();
    }

    /**
     * Adds a {@link PipInputManager.Listener}.
     */
    public void addListener(Listener listener) {
        mListeners.add(listener);
    }

    /**
     * Removes a {@link PipInputManager.Listener}.
     */
    public void removeListener(Listener listener) {
        mListeners.remove(listener);
    }

    /**
     * Gets the size of inputs for PIP.
     *
     * <p>The hidden inputs are not counted.
     *
     * @param availableOnly If {@code true}, it counts only available PIP inputs. Please see {@link
     *        PipInput#isAvailable()} for the details of availability.
     */
    public int getPipInputSize(boolean availableOnly) {
        int count = 0;
        for (PipInput pipInput : mPipInputMap.values()) {
            if (!pipInput.isHidden() && (!availableOnly || pipInput.mAvailable)) {
                ++count;
            }
            if (pipInput.isPassthrough()) {
                TvInputInfo info = pipInput.getInputInfo();
                // Do not count HDMI ports if a CEC device is directly connected to the port.
                if (info.getParentId() != null && !info.isConnectedToHdmiSwitch()) {
                    --count;
                }
            }
        }
        return count;
    }

    /**
     * Gets the list of inputs for PIP..
     *
     * <p>The hidden inputs are excluded.
     *
     * @param availableOnly If true, it returns only available PIP inputs. Please see {@link
     *        PipInput#isAvailable()} for the details of availability.
     */
    public List<PipInput> getPipInputList(boolean availableOnly) {
        List<PipInput> pipInputs = new ArrayList<>();
        List<PipInput> removeInputs = new ArrayList<>();
        for (PipInput pipInput : mPipInputMap.values()) {
            if (!pipInput.isHidden() && (!availableOnly || pipInput.mAvailable)) {
                pipInputs.add(pipInput);
            }
            if (pipInput.isPassthrough()) {
                TvInputInfo info = pipInput.getInputInfo();
                // Do not show HDMI ports if a CEC device is directly connected to the port.
                if (info.getParentId() != null && !info.isConnectedToHdmiSwitch()) {
                    removeInputs.add(mPipInputMap.get(info.getParentId()));
                }
            }
        }
        if (!removeInputs.isEmpty()) {
            pipInputs.removeAll(removeInputs);
        }
        Collections.sort(pipInputs, new Comparator<PipInput>() {
            @Override
            public int compare(PipInput lhs, PipInput rhs) {
                if (!lhs.mIsPassthrough) {
                    return -1;
                }
                if (!rhs.mIsPassthrough) {
                    return 1;
                }
                String a = lhs.getLabel();
                String b = rhs.getLabel();
                return a.compareTo(b);
            }
        });
        return pipInputs;
    }

    /**
     * Returns an PIP input corresponding to {@code channel}.
     */
    public PipInput getPipInput(Channel channel) {
        if (channel == null) {
            return null;
        }
        if (channel.isPassthrough()) {
            return mPipInputMap.get(channel.getInputId());
        } else {
            return mPipInputMap.get(TUNER_INPUT_ID);
        }
    }

    /**
     * Returns true, if {@code channel1} and {@code channel2} belong to the same input. For example,
     * two channels from different tuner inputs are also in the same input "Tuner" from PIP
     * point of view.
     */
    public boolean areInSamePipInput(Channel channel1, Channel channel2) {
        PipInput input1 = getPipInput(channel1);
        PipInput input2 = getPipInput(channel2);
        return input1 != null && input2 != null
                && getPipInput(channel1).equals(getPipInput(channel2));
    }

    private void initializePipInputList() {
        boolean hasTunerInput = false;
        for (TvInputInfo input : mInputManager.getTvInputInfos(false, false)) {
            if (input.isPassthroughInput()) {
                boolean available = mInputManager.getInputState(input)
                        == TvInputManager.INPUT_STATE_CONNECTED;
                mPipInputMap.put(input.getId(), new PipInput(input.getId(), available));
            } else if (!hasTunerInput) {
                hasTunerInput = true;
                boolean available = mChannelTuner.getBrowsableChannelCount() != 0;
                mPipInputMap.put(TUNER_INPUT_ID, new PipInput(TUNER_INPUT_ID, available));
            }
        }
        PipInput input = getPipInput(mChannelTuner.getCurrentChannel());
        if (input != null) {
            input.updateAvailability();
        }
        for (Listener l : mListeners) {
            l.onPipInputListUpdated();
        }
    }

    /**
     * Listeners to notify PIP input state changes.
     */
    public interface Listener {
        /**
         * Called when the state (availability) of PIP inputs is changed.
         */
        void onPipInputStateUpdated();

        /**
         * Called when the list of PIP inputs is changed.
         */
        void onPipInputListUpdated();
    }

    /**
     * Input class for PIP. It has useful methods for PIP handling.
     */
    public class PipInput {
        private final String mInputId;
        private final boolean mIsPassthrough;
        private final TvInputInfo mInputInfo;
        private boolean mAvailable;

        private PipInput(String inputId, boolean available) {
            mInputId = inputId;
            mIsPassthrough = !mInputId.equals(TUNER_INPUT_ID);
            if (mIsPassthrough) {
                mInputInfo = mInputManager.getTvInputInfo(mInputId);
            } else {
                mInputInfo = null;
            }
            mAvailable = available;
        }

        /**
         * Returns the {@link TvInputInfo} object that matches to this PIP input.
         */
        public TvInputInfo getInputInfo() {
            return mInputInfo;
        }

        /**
         * Returns {@code true}, if the input is available for PIP. If a channel of an input is
         * already played or an input is not connected state or there is no browsable channel, the
         * input is unavailable.
         */
        public boolean isAvailable() {
            return mAvailable;
        }

        /**
         * Returns true, if the input is a passthrough TV input.
         */
        public boolean isPassthrough() {
            return mIsPassthrough;
        }

        /**
         * Gets a channel to play in a PIP view.
         */
        public Channel getChannel() {
            if (mIsPassthrough) {
                return Channel.createPassthroughChannel(mInputId);
            } else {
                return mChannelTuner.findNearestBrowsableChannel(
                        Utils.getLastWatchedChannelId(mContext));
            }
        }

        /**
         * Gets a label of the input.
         */
        public String getLabel() {
            if (mIsPassthrough) {
                return mInputInfo.loadLabel(mContext).toString();
            } else {
                return mContext.getString(R.string.input_selector_tuner_label);
            }
        }

        /**
         * Gets a long label including a customized label.
         */
        public String getLongLabel() {
            if (mIsPassthrough) {
                String customizedLabel = Utils.loadLabel(mContext, mInputInfo);
                String label = getLabel();
                if (label.equals(customizedLabel)) {
                    return customizedLabel;
                }
                return customizedLabel + " (" + label + ")";
            } else {
                return mContext.getString(R.string.input_long_label_for_tuner);
            }
        }

        /**
         * Updates availability. It returns true, if availability is changed.
         */
        private void updateAvailability() {
            boolean available;
            // current playing input cannot be available for PIP.
            Channel currentChannel = mChannelTuner.getCurrentChannel();
            if (mIsPassthrough) {
                if (currentChannel != null && currentChannel.getInputId().equals(mInputId)) {
                    available = false;
                } else {
                    available = mInputManager.getInputState(mInputId)
                            == TvInputManager.INPUT_STATE_CONNECTED;
                }
            } else {
                if (currentChannel != null && !currentChannel.isPassthrough()) {
                    available = false;
                } else {
                    available = mChannelTuner.getBrowsableChannelCount() > 0;
                }
            }
            if (mAvailable != available) {
                mAvailable = available;
                for (Listener l : mListeners) {
                    l.onPipInputStateUpdated();
                }
            }
        }

        private boolean isHidden() {
            // mInputInfo is null for the tuner input and it's always visible.
            return mInputInfo != null && mInputInfo.isHidden(mContext);
        }
    }
}
