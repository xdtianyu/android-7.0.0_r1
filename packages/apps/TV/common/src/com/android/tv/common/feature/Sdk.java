/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tv.common.feature;

import android.content.Context;
import android.os.Build;
import android.support.v4.os.BuildCompat;

/**
 *  Holder for SDK version features
 */
public class Sdk {

    public static Feature N_PRE_2_OR_HIGHER =
            new SdkPreviewVersionFeature(Build.VERSION_CODES.M, 2, true);

    private static class SdkPreviewVersionFeature implements Feature {
        private final int mVersionCode;
        private final int mPreviewCode;
        private final boolean mAllowHigherPreview;

        private SdkPreviewVersionFeature(int versionCode, int previewCode,
                boolean allowHigerPreview) {
            mVersionCode = versionCode;
            mPreviewCode = previewCode;
            mAllowHigherPreview = allowHigerPreview;
        }

        @Override
        public boolean isEnabled(Context context) {
            try {
                if (mAllowHigherPreview) {
                    return Build.VERSION.SDK_INT == mVersionCode
                            && Build.VERSION.PREVIEW_SDK_INT >= mPreviewCode;
                } else {
                    return Build.VERSION.SDK_INT == mVersionCode
                            && Build.VERSION.PREVIEW_SDK_INT == mPreviewCode;
                }
            } catch (NoSuchFieldError e) {
                return false;
            }
        }
    }

    public static Feature AT_LEAST_N = new Feature() {
        @Override
        public boolean isEnabled(Context context) {
            return BuildCompat.isAtLeastN();
        }
    };

    private Sdk() {}
}
