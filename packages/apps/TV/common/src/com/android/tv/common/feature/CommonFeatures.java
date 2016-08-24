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

import static com.android.tv.common.feature.EngOnlyFeature.ENG_ONLY_FEATURE;
import static com.android.tv.common.feature.FeatureUtils.OR;
import static com.android.tv.common.feature.TestableFeature.createTestableFeature;

/**
 * List of {@link Feature} that affect more than just the Live TV app.
 *
 * <p>Remove the {@code Feature} once it is launched.
 */
public class CommonFeatures {
    /**
     * DVR
     *
     * <p>See <a href="https://goto.google.com/atv-dvr-onepager">go/atv-dvr-onepager</a>
     */
    public static TestableFeature DVR = createTestableFeature(
            OR(ENG_ONLY_FEATURE, Sdk.N_PRE_2_OR_HIGHER));

    /**
     * USE_SW_CODEC_FOR_SD
     *
     * Prefer software based codec for SD channels.
     */
    public static Feature USE_SW_CODEC_FOR_SD = new PropertyFeature("use_sw_codec_for_sd", true);
}
