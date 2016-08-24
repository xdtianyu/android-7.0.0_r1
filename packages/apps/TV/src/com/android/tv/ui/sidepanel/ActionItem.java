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

import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.tv.R;

public abstract class ActionItem extends Item {
    private final String mTitle;
    private final String mDescription;
    private final int mIconId;

    public ActionItem(String title) {
        this(title, null, 0);
    }

    public ActionItem(String title, String description) {
        this(title, description, 0);
    }

    public ActionItem(String title, int iconId) {
        this(title, null, iconId);
    }

    public ActionItem(String title, String description, int iconId) {
        mTitle = title;
        mDescription = description;
        mIconId = iconId;
    }

    @Override
    protected int getResourceId() {
        return R.layout.option_item_action;
    }

    @Override
    protected void onBind(View view) {
        super.onBind(view);
        TextView titleView = (TextView) view.findViewById(R.id.title);
        titleView.setText(mTitle);
        TextView descriptionView = (TextView) view.findViewById(R.id.description);
        if (mDescription != null) {
            descriptionView.setVisibility(View.VISIBLE);
            descriptionView.setText(mDescription);
        } else {
            descriptionView.setVisibility(View.GONE);
        }
        ImageView iconView = (ImageView) view.findViewById(R.id.icon);
        if (mIconId != 0) {
            iconView.setVisibility(View.VISIBLE);
            iconView.setImageResource(mIconId);
        } else {
            iconView.setVisibility(View.GONE);
        }
    }
}