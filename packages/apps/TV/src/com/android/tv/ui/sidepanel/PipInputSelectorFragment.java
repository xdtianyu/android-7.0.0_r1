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

import android.media.tv.TvInputInfo;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.tv.R;
import com.android.tv.util.PipInputManager;
import com.android.tv.util.PipInputManager.PipInput;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class PipInputSelectorFragment extends SideFragment {
    private static final String TAG = "PipInputSelector";
    private static final String TRACKER_LABEL = "PIP input source";

    private final List<Item> mInputItems = new ArrayList<>();
    private PipInputManager mPipInputManager;
    private PipInput mInitialPipInput;
    private boolean mSelected;

    private final PipInputManager.Listener mPipInputListener = new PipInputManager.Listener() {
        @Override
        public void onPipInputStateUpdated() {
            notifyDataSetChanged();
        }

        @Override
        public void onPipInputListUpdated() {
            refreshInputList();
            setItems(mInputItems);
        }
    };

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        mPipInputManager = getMainActivity().getPipInputManager();
        mPipInputManager.addListener(mPipInputListener);
        getMainActivity().startShrunkenTvView(false, false);
        return super.onCreateView(inflater, container, savedInstanceState);
    }

    @Override
    public void onStart() {
        super.onStart();
        mInitialPipInput = mPipInputManager.getPipInput(getMainActivity().getPipChannel());
        if (mInitialPipInput == null) {
            Log.w(TAG, "PIP should be on");
            closeFragment();
        }
        int count = 0;
        for (Item item : mInputItems) {
            InputItem inputItem = (InputItem) item;
            if (Objects.equals(inputItem.mPipInput, mInitialPipInput)) {
                setSelectedPosition(count);
                break;
            }
            ++count;
        }
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mPipInputManager.removeListener(mPipInputListener);
        if (!mSelected) {
            getMainActivity().tuneToChannelForPip(mInitialPipInput.getChannel());
        }
        getMainActivity().endShrunkenTvView();
    }

    @Override
    protected String getTitle() {
        return getString(R.string.side_panel_title_pip_input_source);
    }

    @Override
    public String getTrackerLabel() {
        return TRACKER_LABEL;
    }

    @Override
    protected List<Item> getItemList() {
        refreshInputList();
        return mInputItems;
    }

    private void refreshInputList() {
        mInputItems.clear();
        for (PipInput input : mPipInputManager.getPipInputList(false)) {
            mInputItems.add(new InputItem(input));
        }
    }

    private class InputItem extends RadioButtonItem {
        private final PipInput mPipInput;

        private InputItem(PipInput input) {
            super(input.getLongLabel());
            mPipInput = input;
            setEnabled(isAvailable());
        }

        @Override
        protected void onUpdate() {
            super.onUpdate();
            setEnabled(mPipInput.isAvailable());
            setChecked(mPipInput == mInitialPipInput);
        }

        @Override
        protected void onFocused() {
            super.onFocused();
            if (isEnabled()) {
                getMainActivity().tuneToChannelForPip(mPipInput.getChannel());
            }
        }

        @Override
        protected void onSelected() {
            super.onSelected();
            if (isEnabled()) {
                mSelected = true;
                closeFragment();
            }
        }

        private boolean isAvailable() {
            if (!mPipInput.isAvailable()) {
                return false;
            }

            // If this input shares the same parent with the current main input, you cannot select
            // it. (E.g. two HDMI CEC devices that are connected to HDMI port 1 through an A/V
            // receiver.)
            PipInput pipInput = mPipInputManager.getPipInput(getMainActivity().getCurrentChannel());
            if (pipInput == null) {
                return false;
            }
            TvInputInfo mainInputInfo = pipInput.getInputInfo();
            TvInputInfo pipInputInfo = mPipInput.getInputInfo();
            return mainInputInfo == null || pipInputInfo == null
                    || !TextUtils.equals(mainInputInfo.getId(), pipInputInfo.getId())
                    && !TextUtils.equals(mainInputInfo.getParentId(), pipInputInfo.getParentId());
        }
    }
}
