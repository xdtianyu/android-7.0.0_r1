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

package com.android.tv.ui.sidepanel;

import android.media.tv.TvTrackInfo;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.tv.R;
import com.android.tv.util.CaptionSettings;
import com.android.tv.util.Utils;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class ClosedCaptionFragment extends SideFragment {
    private static final String TRACKER_LABEL ="closed caption" ;
    private boolean mResetClosedCaption;
    private int mClosedCaptionOption;
    private String mClosedCaptionLanguage;
    private String mClosedCaptionTrackId;
    private ClosedCaptionOptionItem mSelectedItem;
    private List<Item> mItems;
    private boolean mPaused;

    public ClosedCaptionFragment() {
        super(KeyEvent.KEYCODE_CAPTIONS, KeyEvent.KEYCODE_S);
    }

    @Override
    protected String getTitle() {
        return getString(R.string.side_panel_title_closed_caption);
    }

    @Override
    public String getTrackerLabel() {
        return TRACKER_LABEL;
    }

    @Override
    protected List<Item> getItemList() {
        CaptionSettings captionSettings = getMainActivity().getCaptionSettings();
        mResetClosedCaption = true;
        mClosedCaptionOption = captionSettings.getEnableOption();
        mClosedCaptionLanguage = captionSettings.getLanguage();
        mClosedCaptionTrackId = captionSettings.getTrackId();

        mItems = new ArrayList<>();
        mSelectedItem = null;

        List<TvTrackInfo> tracks = getMainActivity().getTracks(TvTrackInfo.TYPE_SUBTITLE);
        if (tracks != null && !tracks.isEmpty()) {
            String trackId = captionSettings.isEnabled() ?
                    getMainActivity().getSelectedTrack(TvTrackInfo.TYPE_SUBTITLE) : null;
            boolean isEnabled = trackId != null;

            ClosedCaptionOptionItem item = new ClosedCaptionOptionItem(
                    getString(R.string.closed_caption_option_item_off),
                    CaptionSettings.OPTION_OFF, null, null);
            // Pick 'Off' as default because we may fail to find the matching language.
            mSelectedItem = item;
            if (!isEnabled) {
                item.setChecked(true);
            }
            mItems.add(item);

            for (final TvTrackInfo track : tracks) {
                item = new ClosedCaptionOptionItem(getLabel(track),
                        CaptionSettings.OPTION_ON, track.getId(), track.getLanguage());
                if (isEnabled && track.getId().equals(trackId)) {
                    item.setChecked(true);
                    mSelectedItem = item;
                }
                mItems.add(item);
            }
        }
        if (getMainActivity().hasCaptioningSettingsActivity()) {
            mItems.add(new ActionItem(getString(R.string.closed_caption_system_settings),
                    getString(R.string.closed_caption_system_settings_description)) {
                @Override
                protected void onSelected() {
                    getMainActivity().startSystemCaptioningSettingsActivity();
                }

                @Override
                protected void onFocused() {
                    super.onFocused();
                    if (!mPaused && mSelectedItem != null) {
                        getMainActivity().selectSubtitleTrack(
                                mSelectedItem.mOption, mSelectedItem.mTrackId);
                    }
                }
            });
        }
        return mItems;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return super.onCreateView(inflater, container, savedInstanceState);
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mPaused) {
            // Apply system's closed caption settings to the UI.
            CaptionSettings captionSettings = getMainActivity().getCaptionSettings();
            mClosedCaptionOption = CaptionSettings.OPTION_SYSTEM;
            mClosedCaptionLanguage = captionSettings.getSystemLanguage();
            ClosedCaptionOptionItem selectedItem = null;
            if (captionSettings.isSystemSettingEnabled()) {
                for (Item item : mItems) {
                    if (!(item instanceof ClosedCaptionOptionItem)) {
                        continue;
                    }
                    ClosedCaptionOptionItem captionItem = (ClosedCaptionOptionItem) item;
                    if (Utils.isEqualLanguage(captionItem.mLanguage, mClosedCaptionLanguage)) {
                        selectedItem = captionItem;
                        break;
                    }
                }
            }
            if (mSelectedItem != null) {
                mSelectedItem.setChecked(false);
            }
            if (selectedItem == null && mItems.get(0) instanceof ClosedCaptionOptionItem) {
                selectedItem = (ClosedCaptionOptionItem) mItems.get(0);
            }
            if (selectedItem != null) {
                selectedItem.setChecked(true);
            }
            // We shouldn't call MainActivity.selectSubtitleTrack() here because
            //   1. Tracks are not available because video is just started at this moment.
            //   2. MainActivity will apply system settings when video's tracks are available.
            mSelectedItem = selectedItem;
        }
        mPaused = false;
    }

    @Override
    public void onPause() {
        super.onPause();
        mPaused = true;
    }

    @Override
    public void onDestroyView() {
        if (mResetClosedCaption) {
            getMainActivity().selectSubtitleLanguage(mClosedCaptionOption, mClosedCaptionLanguage,
                    mClosedCaptionTrackId);
        }
        super.onDestroyView();
    }

    private String getLabel(TvTrackInfo track) {
        if (track.getLanguage() != null) {
            return new Locale(track.getLanguage()).getDisplayName();
        }
        return getString(R.string.default_language);
    }

    private class ClosedCaptionOptionItem extends RadioButtonItem {
        private final int mOption;
        private final String mTrackId;
        private final String mLanguage;

        private ClosedCaptionOptionItem(String title, int option, String trackId, String language) {
            super(title);
            mOption = option;
            mTrackId = trackId;
            mLanguage = language;
        }

        @Override
        protected void onSelected() {
            super.onSelected();
            mSelectedItem = this;
            getMainActivity().selectSubtitleTrack(mOption, mTrackId);
            mResetClosedCaption = false;
            closeFragment();
        }

        @Override
        protected void onFocused() {
            super.onFocused();
            getMainActivity().selectSubtitleTrack(mOption, mTrackId);
        }
    }
}
