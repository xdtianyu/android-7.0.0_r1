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
package android.support.car.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Path;
import android.util.AttributeSet;
import android.view.View;
import android.widget.FrameLayout;

/**
 * FrameLayout that enables the user to set a Path that the view will be clipped to
 *
 * From GoogleSearch/com.google.android.shared.ui/CircularClipAnimation
 */
public class ClippableFrameLayout extends FrameLayout implements PathClippingView {
    private Path mClipPath;

    public ClippableFrameLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void setClipPath(Path clipPath) {
        mClipPath = clipPath;
        setClipToPadding(true);
        setClipChildren(true);
        setLayerType(View.LAYER_TYPE_SOFTWARE, null);
        setWillNotDraw(false);
        invalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (mClipPath != null) {
            canvas.clipPath(mClipPath);
        }
        super.onDraw(canvas);
    }
}
