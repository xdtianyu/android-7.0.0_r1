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

package com.android.tv.ui;

import android.content.Context;
import android.media.tv.TvView;
import android.util.AttributeSet;

/**
 * A TvView class for application layer when multiple windows are being used in the app.
 * <p>
 * Once an app starts using additional window like SubPanel and it gets window focus, the
 * {@link android.media.tv.TvView#setMain()} does not work because its implementation assumes that
 * the app uses only application layer.
 * TODO: remove this class once the TvView.setMain() is revisited.
 * </p>
 */
public class AppLayerTvView extends TvView {
    public AppLayerTvView(Context context) {
        super(context);
    }

    public AppLayerTvView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public AppLayerTvView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @Override
    public boolean hasWindowFocus() {
        return true;
    }
}
