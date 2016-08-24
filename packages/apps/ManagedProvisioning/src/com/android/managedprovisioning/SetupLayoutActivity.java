/*
 * Copyright 2015, The Android Open Source Project
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

package com.android.managedprovisioning;

import android.app.Activity;
import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.managedprovisioning.common.Utils;
import com.android.setupwizardlib.SetupWizardLayout;
import com.android.setupwizardlib.util.SystemBarHelper;
import com.android.setupwizardlib.view.NavigationBar;
import com.android.setupwizardlib.view.NavigationBar.NavigationBarListener;

/**
 * Base class for setting up the layout.
 */
public abstract class SetupLayoutActivity extends Activity implements NavigationBarListener {
    protected final Utils mUtils = new Utils();

    protected Button mNextButton;
    protected Button mBackButton;

    public static final int NEXT_BUTTON_EMPTY_LABEL = 0;

    protected void maybeSetLogoAndMainColor(Integer mainColor) {
        // null means the default value
        if (mainColor == null) {
            mainColor = getResources().getColor(R.color.orange);
        }
        // We should always use a value of 255 for the alpha.
        mainColor = Color.argb(255, Color.red(mainColor), Color.green(mainColor),
                Color.blue(mainColor));
        Window window = getWindow();
        window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
        window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);

        SetupWizardLayout layout = (SetupWizardLayout) findViewById(R.id.setup_wizard_layout);

        layout.setIllustration(new HeaderDrawable(this, mainColor));
        layout.setLayoutBackground(new ColorDrawable(mainColor));
        layout.setProgressBarColor(ColorStateList.valueOf(mainColor));

        final TextView titleView = (TextView) findViewById(R.id.suw_layout_title);
        if (mUtils.isBrightColor(mainColor)) {
            titleView.setTextColor(Color.BLACK);
        } else {
            titleView.setTextColor(Color.WHITE);
        }
        if (!mUtils.isUserSetupCompleted(this)) {
            SystemBarHelper.hideSystemBars(window);
        }
    }

    public void initializeLayoutParams(int layoutResourceId, int headerResourceId,
            boolean showProgressBar) {
        setContentView(layoutResourceId);
        SetupWizardLayout layout = (SetupWizardLayout) findViewById(R.id.setup_wizard_layout);
        layout.setHeaderText(headerResourceId);
        if (showProgressBar) {
            layout.showProgressBar();
        }
        setupNavigationBar(layout.getNavigationBar());
    }

    private void setupNavigationBar(NavigationBar bar) {
        bar.setNavigationBarListener(this);
        mNextButton = bar.getNextButton();
        mBackButton = bar.getBackButton();
    }

    public void configureNavigationButtons(int nextButtonResourceId, int nextButtonVisibility,
            int backButtonVisibility) {
        if (nextButtonResourceId != NEXT_BUTTON_EMPTY_LABEL) {
            mNextButton.setText(nextButtonResourceId);
        }
        mNextButton.setVisibility(nextButtonVisibility);
        mBackButton.setVisibility(backButtonVisibility);
    }

    @Override
    public void onNavigateBack() {
        onBackPressed();
    }

    @Override
    public void onNavigateNext() {
    }

    private class HeaderDrawable extends Drawable {
        private Activity mActivity;
        private int mMainColor;

        HeaderDrawable(Activity a, int mainColor) {
            mActivity = a;
            mMainColor = mainColor;
        }

        @Override
        public void draw(Canvas canvas) {
            Drawable logo = LogoUtils.getOrganisationLogo(mActivity);
            // At this point, the logo has already been resized.
            int logoWidth = logo.getIntrinsicWidth();
            int logoHeight = logo.getIntrinsicHeight();
            Resources resources = mActivity.getResources();

            int logoPaddingLeftRight = (int) resources
                    .getDimension(R.dimen.logo_padding_left_right);
            int logoPaddingBottom = (int) resources
                    .getDimension(R.dimen.logo_padding_bottom);
            int totalWidth = getIntrinsicWidth();
            int totalHeight = getIntrinsicHeight();

            // By default, the drawable is materialized: it is not a solid color. Draw a white
            // rectangle over the whole drawable so that it is a solid color.
            Paint paint = new Paint();
            paint.setColor(resources.getColor(R.color.white));
            canvas.drawRect(0, 0, totalWidth, totalHeight, paint);

            // Draw the logo.
            if (shouldDrawLogoOnLeftSide()) {
                logo.setBounds(logoPaddingLeftRight,
                        totalHeight - logoPaddingBottom - logoHeight,
                        logoPaddingLeftRight + logoWidth,
                        totalHeight - logoPaddingBottom);
            } else {
                logo.setBounds(totalWidth - logoPaddingLeftRight - logoWidth,
                        totalHeight - logoPaddingBottom - logoHeight,
                        totalWidth - logoPaddingLeftRight,
                        totalHeight - logoPaddingBottom);
            }
            logo.draw(canvas);

        }

        @Override
        public int getIntrinsicHeight() {
            if (mActivity.getResources().getBoolean(R.bool.suwUseTabletLayout)) {
                return (int) mActivity.getResources()
                        .getDimension(R.dimen.suw_tablet_illustration_height);
            }
            return getScreenWidth() * 9 / 20;
        }

        @Override
        public int getIntrinsicWidth() {
            return getScreenWidth();
        }

        @Override
        public int getOpacity() {
            return PixelFormat.OPAQUE;
        }

        @Override
        public void setAlpha(int alpha) {
            //ignore
        }

        @Override
        public void setColorFilter(ColorFilter cf) {
            // ignore
        }

        private int getScreenWidth() {
            DisplayMetrics metrics = new DisplayMetrics();
            mActivity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
            return metrics.widthPixels;
        }

        private boolean shouldDrawLogoOnLeftSide() {
            // for a tablet layout, the logo should be in the bottom left
            boolean result = useTabletLayout();
            // for a right-to-left language, reverse it.
            if (mActivity.getResources().getConfiguration().getLayoutDirection()
                    == View.LAYOUT_DIRECTION_RTL) {
                result = !result;
            }
            return result;
        }

        private boolean useTabletLayout() {
            return mActivity.getResources().getBoolean(R.bool.suwUseTabletLayout);
        }
    }
}
