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
import android.util.AttributeSet;

import com.android.tv.R;
import com.android.tv.data.Channel;

/**
 * A view to render a guide card.
 */
public class SimpleCardView extends BaseCardView<Channel> {
    private static final String TAG = "GuideCardView";
    private static final boolean DEBUG = false;
    private final float mCardHeight;

    public SimpleCardView(Context context) {
        this(context, null, 0);
    }

    public SimpleCardView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SimpleCardView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mCardHeight = getResources().getDimension(R.dimen.card_layout_height);
    }

    @Override
    protected float getCardHeight() {
        return mCardHeight;
    }
}
