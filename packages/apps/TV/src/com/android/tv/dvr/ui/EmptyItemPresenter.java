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

package com.android.tv.dvr.ui;

import android.content.res.Resources;
import android.graphics.Color;
import android.support.v17.leanback.widget.Presenter;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.TextView;

import com.android.tv.R;
import com.android.tv.TvApplication;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.util.Utils;

/**
 * Shows the item "NONE".  Used for rows with now items.
 */
public class EmptyItemPresenter extends Presenter {

    private final DvrBrowseFragment mMainFragment;

    public EmptyItemPresenter(DvrBrowseFragment mainFragment) {
        mMainFragment = mainFragment;
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent) {
        TextView view = new TextView(parent.getContext());
        Resources resources = view.getResources();
        view.setLayoutParams(new ViewGroup.LayoutParams(
                resources.getDimensionPixelSize(R.dimen.dvr_card_layout_width),
                resources.getDimensionPixelSize(R.dimen.dvr_card_layout_width)));
        view.setFocusable(true);
        view.setFocusableInTouchMode(true);
        view.setBackgroundColor(
                Utils.getColor(mMainFragment.getResources(), R.color.setup_background));
        view.setTextColor(Color.WHITE);
        view.setGravity(Gravity.CENTER);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(ViewHolder viewHolder, Object recording) {
        ((TextView) viewHolder.view).setText(
                viewHolder.view.getContext().getString(R.string.dvr_msg_no_recording_on_the_row));
    }

    @Override
    public void onUnbindViewHolder(ViewHolder viewHolder) { }
}
