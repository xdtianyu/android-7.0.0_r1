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
import android.text.TextUtils;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.TvOptionsManager;
import com.android.tv.ui.TvViewUiManager;
import com.android.tv.ui.sidepanel.PipInputSelectorFragment;
import com.android.tv.util.PipInputManager.PipInput;
import com.android.tv.util.TvSettings;

import java.util.ArrayList;
import java.util.List;

/*
 * An adapter of PIP options.
 */
public class PipOptionsRowAdapter extends OptionsRowAdapter {
    private static final int[] DRAWABLE_ID_FOR_LAYOUT = {
            R.drawable.ic_pip_option_layout1,
            R.drawable.ic_pip_option_layout2,
            R.drawable.ic_pip_option_layout3,
            R.drawable.ic_pip_option_layout4,
            R.drawable.ic_pip_option_layout5 };

    private final TvOptionsManager mTvOptionsManager;
    private final TvViewUiManager mTvViewUiManager;

    public PipOptionsRowAdapter(Context context) {
        super(context);
        mTvOptionsManager = getMainActivity().getTvOptionsManager();
        mTvViewUiManager = getMainActivity().getTvViewUiManager();
    }

    @Override
    protected List<MenuAction> createActions() {
        List<MenuAction> actionList = new ArrayList<>();
        actionList.add(MenuAction.PIP_SELECT_INPUT_ACTION);
        actionList.add(MenuAction.PIP_SWAP_ACTION);
        actionList.add(MenuAction.PIP_SOUND_ACTION);
        actionList.add(MenuAction.PIP_LAYOUT_ACTION);
        actionList.add(MenuAction.PIP_SIZE_ACTION);
        for (MenuAction action : actionList) {
            setOptionChangedListener(action);
        }
        return actionList;
    }

    @Override
    public boolean updateActions() {
        boolean changed = false;
        if (updateSelectInputAction()) {
            changed = true;
        }
        if (updateLayoutAction()) {
            changed = true;
        }
        if (updateSizeAction()) {
            changed = true;
        }
        return changed;
    }

    private boolean updateSelectInputAction() {
        String oldInputLabel = mTvOptionsManager.getOptionString(TvOptionsManager.OPTION_PIP_INPUT);

        MainActivity tvActivity = getMainActivity();
        PipInput newInput = tvActivity.getPipInputManager().getPipInput(tvActivity.getPipChannel());
        String newInputLabel = newInput == null ? null : newInput.getLabel();

        if (!TextUtils.equals(oldInputLabel, newInputLabel)) {
            mTvOptionsManager.onPipInputChanged(newInputLabel);
            return true;
        }
        return false;
    }

    private boolean updateLayoutAction() {
        return MenuAction.PIP_LAYOUT_ACTION.setDrawableResId(
            DRAWABLE_ID_FOR_LAYOUT[mTvViewUiManager.getPipLayout()]);
    }

    private boolean updateSizeAction() {
        boolean oldEnabled = MenuAction.PIP_SIZE_ACTION.isEnabled();
        boolean newEnabled = mTvViewUiManager.getPipLayout() != TvSettings.PIP_LAYOUT_SIDE_BY_SIDE;
        if (oldEnabled != newEnabled) {
            MenuAction.PIP_SIZE_ACTION.setEnabled(newEnabled);
            return true;
        }
        return false;
    }

    @Override
    protected void executeAction(int type) {
        switch (type) {
            case TvOptionsManager.OPTION_PIP_INPUT:
                getMainActivity().getOverlayManager().getSideFragmentManager().show(
                        new PipInputSelectorFragment());
                break;
            case TvOptionsManager.OPTION_PIP_SWAP:
                getMainActivity().swapPip();
                break;
            case TvOptionsManager.OPTION_PIP_SOUND:
                getMainActivity().togglePipSoundMode();
                break;
            case TvOptionsManager.OPTION_PIP_LAYOUT:
                int oldLayout = mTvViewUiManager.getPipLayout();
                int newLayout = (oldLayout + 1) % (TvSettings.PIP_LAYOUT_LAST + 1);
                mTvViewUiManager.setPipLayout(newLayout, true);
                MenuAction.PIP_LAYOUT_ACTION.setDrawableResId(DRAWABLE_ID_FOR_LAYOUT[newLayout]);
                break;
            case TvOptionsManager.OPTION_PIP_SIZE:
                int oldSize = mTvViewUiManager.getPipSize();
                int newSize = (oldSize + 1) % (TvSettings.PIP_SIZE_LAST + 1);
                mTvViewUiManager.setPipSize(newSize, true);
                break;
        }
    }
}
