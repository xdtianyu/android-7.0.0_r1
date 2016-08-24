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

package com.android.tv;

import android.content.Context;
import android.media.tv.TvTrackInfo;
import android.util.SparseArray;

import com.android.tv.data.DisplayMode;
import com.android.tv.util.TvSettings;
import com.android.tv.util.TvSettings.PipLayout;
import com.android.tv.util.TvSettings.PipSize;
import com.android.tv.util.TvSettings.PipSound;

import java.util.Locale;

/**
 * The TvOptionsManager is responsible for keeping track of current TV options such as closed
 * captions and display mode. Can be also used to create MenuAction items to control such options.
 */
public class TvOptionsManager {
    public static final int OPTION_CLOSED_CAPTIONS = 0;
    public static final int OPTION_DISPLAY_MODE = 1;
    public static final int OPTION_IN_APP_PIP = 2;
    public static final int OPTION_SYSTEMWIDE_PIP = 3;
    public static final int OPTION_MULTI_AUDIO = 4;
    public static final int OPTION_MORE_CHANNELS = 5;
    public static final int OPTION_SETTINGS = 6;

    public static final int OPTION_PIP_INPUT = 100;
    public static final int OPTION_PIP_SWAP = 101;
    public static final int OPTION_PIP_SOUND = 102;
    public static final int OPTION_PIP_LAYOUT = 103 ;
    public static final int OPTION_PIP_SIZE = 104;

    private final Context mContext;
    private final SparseArray<OptionChangedListener> mOptionChangedListeners = new SparseArray<>();

    private String mClosedCaptionsLanguage;
    private int mDisplayMode;
    private boolean mPip;
    private String mMultiAudio;
    private String mPipInput;
    private boolean mPipSwap;
    @PipSound private int mPipSound;
    @PipLayout private int mPipLayout;
    @PipSize private int mPipSize;

    public TvOptionsManager(Context context) {
        mContext = context;
    }

    public String getOptionString(int option) {
        switch (option) {
            case OPTION_CLOSED_CAPTIONS:
                if (mClosedCaptionsLanguage == null) {
                    return mContext.getString(R.string.closed_caption_option_item_off);
                }
                return new Locale(mClosedCaptionsLanguage).getDisplayName();
            case OPTION_DISPLAY_MODE:
                return ((MainActivity) mContext).getTvViewUiManager()
                        .isDisplayModeAvailable(mDisplayMode)
                        ? DisplayMode.getLabel(mDisplayMode, mContext)
                        : DisplayMode.getLabel(DisplayMode.MODE_NORMAL, mContext);
            case OPTION_IN_APP_PIP:
                return mContext.getString(
                        mPip ? R.string.options_item_pip_on : R.string.options_item_pip_off);
            case OPTION_MULTI_AUDIO:
                return mMultiAudio;
            case OPTION_PIP_INPUT:
                return mPipInput;
            case OPTION_PIP_SWAP:
                return mContext.getString(mPipSwap ? R.string.pip_options_item_swap_on
                        : R.string.pip_options_item_swap_off);
            case OPTION_PIP_SOUND:
                if (mPipSound == TvSettings.PIP_SOUND_MAIN) {
                    return mContext.getString(R.string.pip_options_item_sound_main);
                } else if (mPipSound == TvSettings.PIP_SOUND_PIP_WINDOW) {
                    return mContext.getString(R.string.pip_options_item_sound_pip_window);
                }
                break;
            case OPTION_PIP_LAYOUT:
                if (mPipLayout == TvSettings.PIP_LAYOUT_BOTTOM_RIGHT) {
                    return mContext.getString(R.string.pip_options_item_layout_bottom_right);
                } else if (mPipLayout == TvSettings.PIP_LAYOUT_TOP_RIGHT) {
                    return mContext.getString(R.string.pip_options_item_layout_top_right);
                } else if (mPipLayout == TvSettings.PIP_LAYOUT_TOP_LEFT) {
                    return mContext.getString(R.string.pip_options_item_layout_top_left);
                } else if (mPipLayout == TvSettings.PIP_LAYOUT_BOTTOM_LEFT) {
                    return mContext.getString(R.string.pip_options_item_layout_bottom_left);
                } else if (mPipLayout == TvSettings.PIP_LAYOUT_SIDE_BY_SIDE) {
                    return mContext.getString(R.string.pip_options_item_layout_side_by_side);
                }
                break;
            case OPTION_PIP_SIZE:
                if (mPipSize == TvSettings.PIP_SIZE_BIG) {
                    return mContext.getString(R.string.pip_options_item_size_big);
                } else if (mPipSize == TvSettings.PIP_SIZE_SMALL) {
                    return mContext.getString(R.string.pip_options_item_size_small);
                }
                break;
        }
        return "";
    }

    public void onClosedCaptionsChanged(TvTrackInfo track) {
        mClosedCaptionsLanguage = (track == null) ? null
                : (track.getLanguage() != null) ? track.getLanguage()
                        : mContext.getString(R.string.default_language);
        notifyOptionChanged(OPTION_CLOSED_CAPTIONS);
    }

    public void onDisplayModeChanged(int displayMode) {
        mDisplayMode = displayMode;
        notifyOptionChanged(OPTION_DISPLAY_MODE);
    }

    public void onPipChanged(boolean pip) {
        mPip = pip;
        notifyOptionChanged(OPTION_IN_APP_PIP);
    }

    public void onMultiAudioChanged(String multiAudio) {
        mMultiAudio = multiAudio;
        notifyOptionChanged(OPTION_MULTI_AUDIO);
    }

    public void onPipInputChanged(String pipInput) {
        mPipInput = pipInput;
        notifyOptionChanged(OPTION_PIP_INPUT);
    }

    public void onPipSwapChanged(boolean pipSwap) {
        mPipSwap = pipSwap;
        notifyOptionChanged(OPTION_PIP_SWAP);
    }

    public void onPipSoundChanged(@PipSound int pipSound) {
        mPipSound = pipSound;
        notifyOptionChanged(OPTION_PIP_SOUND);
    }

    public void onPipLayoutChanged(@PipLayout int pipLayout) {
        mPipLayout = pipLayout;
        notifyOptionChanged(OPTION_PIP_LAYOUT);
    }

    public void onPipSizeChanged(@PipSize int pipSize) {
        mPipSize = pipSize;
        notifyOptionChanged(OPTION_PIP_SIZE);
    }

    private void notifyOptionChanged(int option) {
        OptionChangedListener listener = mOptionChangedListeners.get(option);
        if (listener != null) {
            listener.onOptionChanged(getOptionString(option));
        }
    }

    public void setOptionChangedListener(int option, OptionChangedListener listener) {
        mOptionChangedListeners.put(option, listener);
    }

    /**
     * An interface used to monitor option changes.
     */
    public interface OptionChangedListener {
        void onOptionChanged(String newOption);
    }
}
