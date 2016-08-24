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
 * limitations under the License
 */

package com.android.tv;

import static com.android.tv.common.feature.EngOnlyFeature.ENG_ONLY_FEATURE;
import static com.android.tv.common.feature.FeatureUtils.AND;
import static com.android.tv.common.feature.FeatureUtils.ON;
import static com.android.tv.common.feature.FeatureUtils.OR;

import android.content.Context;
import android.os.Build;
import android.support.annotation.VisibleForTesting;
import android.support.v4.os.BuildCompat;

import com.android.tv.common.feature.Feature;
import com.android.tv.common.feature.GServiceFeature;
import com.android.tv.common.feature.PackageVersionFeature;
import com.android.tv.common.feature.PropertyFeature;
import com.android.tv.util.PermissionUtils;

/**
 * List of {@link Feature} for the Live TV App.
 *
 * <p>Remove the {@code Feature} once it is launched.
 */
public final class Features {
    /**
     * UI for opting in to analytics.
     *
     * <p>Do not turn this on until the splash screen asking existing users to opt-in is launched.
     * See <a href="http://b/20228119">b/20228119</a>
     */
    public static final Feature ANALYTICS_OPT_IN = ENG_ONLY_FEATURE;

    /**
     * Analytics that include sensitive information such as channel or program identifiers.
     *
     * <p>See <a href="http://b/22062676">b/22062676</a>
     */
    public static final Feature ANALYTICS_V2 = AND(ON, ANALYTICS_OPT_IN);

    public static final Feature EPG_SEARCH =
            new PropertyFeature("feature_tv_use_epg_search", false);

    public static final Feature USB_TUNER = new Feature() {

        /**
         * This is special handling just for USB Tuner.
         * It does not require any N API's but relies on a improvements in N for AC3 support
         * After release, change class to this to just be
         * {@link BuildCompat#isAtLeastN()}.
         */
        @Override
        public boolean isEnabled(Context context) {
            return Build.VERSION.SDK_INT > Build.VERSION_CODES.M || BuildCompat.isAtLeastN();
        }

    };

    private static final String PLAY_STORE_PACKAGE_NAME = "com.android.vending";
    private static final int PLAY_STORE_ZIMA_VERSION_CODE = 80441186;
    private static final Feature PLAY_STORE_LINK =
            new PackageVersionFeature(PLAY_STORE_PACKAGE_NAME, PLAY_STORE_ZIMA_VERSION_CODE);

    public static final Feature ONBOARDING_PLAY_STORE = PLAY_STORE_LINK;

    /**
     * A flag which indicates that the on-boarding experience is used or not.
     *
     * <p>See <a href="http://b/24070322">b/24070322</a>
     */
    public static final Feature ONBOARDING_EXPERIENCE = ONBOARDING_PLAY_STORE;

    private static final String GSERVICE_KEY_UNHIDE = "live_channels_unhide";
    /**
     * A flag which indicates that LC app is unhidden even when there is no input.
     */
    public static final Feature UNHIDE = AND(ONBOARDING_EXPERIENCE,
            OR(new GServiceFeature(GSERVICE_KEY_UNHIDE, false), new Feature() {
                @Override
                public boolean isEnabled(Context context) {
                    // If LC app runs as non-system app, we unhide the app.
                    return !PermissionUtils.hasAccessAllEpg(context);
                }
            }));

    @VisibleForTesting
    public static Feature TEST_FEATURE = new PropertyFeature("test_feature", false);

    public static final Feature FETCH_EPG = new PropertyFeature("live_channels_fetch_epg", false);

    private Features() {
    }
}
