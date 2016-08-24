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

package com.android.tv.menu;

import android.content.Context;
import android.media.tv.TvTrackInfo;
import android.support.annotation.VisibleForTesting;
import android.support.v4.os.BuildCompat;

import com.android.tv.Features;
import com.android.tv.R;
import com.android.tv.TvOptionsManager;
import com.android.tv.customization.CustomAction;
import com.android.tv.data.DisplayMode;
import com.android.tv.ui.TvViewUiManager;
import com.android.tv.ui.sidepanel.ClosedCaptionFragment;
import com.android.tv.ui.sidepanel.DisplayModeFragment;
import com.android.tv.ui.sidepanel.MultiAudioFragment;
import com.android.tv.util.PipInputManager;

import java.util.ArrayList;
import java.util.List;

/*
 * An adapter of options.
 */
public class TvOptionsRowAdapter extends CustomizableOptionsRowAdapter {
    private int mPositionPipAction;
    // If mInAppPipAction is false, system-wide PIP is used.
    private boolean mInAppPipAction = true;
    private final Context mContext;

    public TvOptionsRowAdapter(Context context, List<CustomAction> customActions) {
        super(context, customActions);
        mContext = context;
    }

    @Override
    protected List<MenuAction> createBaseActions() {
        List<MenuAction> actionList = new ArrayList<>();
        actionList.add(MenuAction.SELECT_CLOSED_CAPTION_ACTION);
        setOptionChangedListener(MenuAction.SELECT_CLOSED_CAPTION_ACTION);
        actionList.add(MenuAction.SELECT_DISPLAY_MODE_ACTION);
        setOptionChangedListener(MenuAction.SELECT_DISPLAY_MODE_ACTION);
        actionList.add(MenuAction.PIP_IN_APP_ACTION);
        setOptionChangedListener(MenuAction.PIP_IN_APP_ACTION);
        mPositionPipAction = actionList.size() - 1;
        actionList.add(MenuAction.SELECT_AUDIO_LANGUAGE_ACTION);
        setOptionChangedListener(MenuAction.SELECT_AUDIO_LANGUAGE_ACTION);
        if (Features.ONBOARDING_PLAY_STORE.isEnabled(getMainActivity())) {
            actionList.add(MenuAction.MORE_CHANNELS_ACTION);
        }
        actionList.add(MenuAction.SETTINGS_ACTION);

        if (getCustomActions() != null) {
            // Adjust Pip action position which will be changed by applying custom actions.
            for (CustomAction customAction : getCustomActions()) {
                if (customAction.isFront()) {
                    mPositionPipAction++;
                }
            }
        }

        return actionList;
    }

    @Override
    protected boolean updateActions() {
        boolean changed = false;
        if (updatePipAction()) {
            changed = true;
        }
        if (updateMultiAudioAction()) {
            changed = true;
        }
        if (updateDisplayModeAction()) {
            changed = true;
        }
        return changed;
    }

    private boolean updatePipAction() {
        // There are four states.
        // Case 1. The device doesn't even have any input for PIP. (e.g. OTT box without HDMI input)
        //    => Remove the icon.
        // Case 2. The device has one or more inputs for PIP but none of them are currently
        // available.
        //    => Show the icon but disable it.
        // Case 3. The device has one or more available PIP inputs and now it's tuned off.
        //    => Show the icon with "Off".
        // Case 4. The device has one or more available PIP inputs but it's already turned on.
        //    => Show the icon with "On".

        boolean changed = false;

        // Case 1
        PipInputManager pipInputManager = getMainActivity().getPipInputManager();
        if (pipInputManager.getPipInputSize(false) < 2) {
            if (mInAppPipAction) {
                removeAction(mPositionPipAction);
                mInAppPipAction = false;
                if (BuildCompat.isAtLeastN()) {
                    addAction(mPositionPipAction, MenuAction.SYSTEMWIDE_PIP_ACTION);
                }
                return true;
            }
            return false;
        } else {
            if (!mInAppPipAction) {
                removeAction(mPositionPipAction);
                addAction(mPositionPipAction, MenuAction.PIP_IN_APP_ACTION);
                mInAppPipAction = true;
                changed = true;
            }
        }

        // Case 2
        boolean isPipEnabled = getMainActivity().isPipEnabled();
        boolean oldEnabled = MenuAction.PIP_IN_APP_ACTION.isEnabled();
        boolean newEnabled = pipInputManager.getPipInputSize(true) > 0;
        if (oldEnabled != newEnabled) {
            // Should not disable the item if the PIP is already turned on so that the user can
            // force exit it.
            if (newEnabled || !isPipEnabled) {
                MenuAction.PIP_IN_APP_ACTION.setEnabled(newEnabled);
                changed = true;
            }
        }

        // Case 3 & 4 - we just need to update the icon.
        MenuAction.PIP_IN_APP_ACTION.setDrawableResId(
                isPipEnabled ? R.drawable.ic_tvoption_pip : R.drawable.ic_tvoption_pip_off);
        return changed;
    }

    @VisibleForTesting
    boolean updateMultiAudioAction() {
        List<TvTrackInfo> audioTracks = getMainActivity().getTracks(TvTrackInfo.TYPE_AUDIO);
        boolean oldEnabled = MenuAction.SELECT_AUDIO_LANGUAGE_ACTION.isEnabled();
        boolean newEnabled = audioTracks != null && audioTracks.size() > 1;
        if (oldEnabled != newEnabled) {
            MenuAction.SELECT_AUDIO_LANGUAGE_ACTION.setEnabled(newEnabled);
            return true;
        }
        return false;
    }

    private boolean updateDisplayModeAction() {
        TvViewUiManager uiManager = getMainActivity().getTvViewUiManager();
        boolean oldEnabled = MenuAction.SELECT_DISPLAY_MODE_ACTION.isEnabled();
        boolean newEnabled = uiManager.isDisplayModeAvailable(DisplayMode.MODE_FULL)
                || uiManager.isDisplayModeAvailable(DisplayMode.MODE_ZOOM);
        if (oldEnabled != newEnabled) {
            MenuAction.SELECT_DISPLAY_MODE_ACTION.setEnabled(newEnabled);
            return true;
        }
        return false;
    }

    @Override
    protected void executeBaseAction(int type) {
        switch (type) {
            case TvOptionsManager.OPTION_CLOSED_CAPTIONS:
                getMainActivity().getOverlayManager().getSideFragmentManager().show(
                        new ClosedCaptionFragment());
                break;
            case TvOptionsManager.OPTION_DISPLAY_MODE:
                getMainActivity().getOverlayManager().getSideFragmentManager().show(
                        new DisplayModeFragment());
                break;
            case TvOptionsManager.OPTION_IN_APP_PIP:
                getMainActivity().togglePipView();
                break;
            case TvOptionsManager.OPTION_SYSTEMWIDE_PIP:
                getMainActivity().enterPictureInPictureMode();
                break;
            case TvOptionsManager.OPTION_MULTI_AUDIO:
                getMainActivity().getOverlayManager().getSideFragmentManager().show(
                        new MultiAudioFragment());
                break;
            case TvOptionsManager.OPTION_MORE_CHANNELS:
                getMainActivity().showMerchantCollection();
                break;
            case TvOptionsManager.OPTION_SETTINGS:
                getMainActivity().showSettingsFragment();
                break;
        }
    }
}
