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
package com.android.tv.util;

import static com.android.tv.util.TvTrackInfoUtils.getBestTrackInfo;

import android.media.tv.TvTrackInfo;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.tv.testing.ComparatorTester;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * Tests for {@link com.android.tv.util.TvTrackInfoUtils}.
 */
@SmallTest
public class TvTrackInfoUtilsTest extends TestCase {

    private static final String UN_MATCHED_ID = "no matching ID";

    private static final TvTrackInfo INFO_1_EN_1 = create("1", "en", 1);

    private static final TvTrackInfo INFO_2_EN_5 = create("2", "en", 5);

    private static final TvTrackInfo INFO_3_FR_5 = create("3", "fr", 5);

    private static TvTrackInfo create(String id, String fr, int audioChannelCount) {
        return new TvTrackInfo.Builder(TvTrackInfo.TYPE_AUDIO, id)
                .setLanguage(fr)
                .setAudioChannelCount(audioChannelCount)
                .build();
    }

    private static final List<TvTrackInfo> ALL = Arrays.asList(INFO_1_EN_1, INFO_2_EN_5, INFO_3_FR_5);

    public void testGetBestTrackInfo_empty() {
        TvTrackInfo result = getBestTrackInfo(Collections.<TvTrackInfo>emptyList(),
                UN_MATCHED_ID, "en", 1);
        assertEquals("best track ", null, result);
    }

    public void testGetBestTrackInfo_exactMatch() {
        TvTrackInfo result = getBestTrackInfo(ALL, "1", "en", 1);
        assertEquals("best track ", INFO_1_EN_1, result);
    }

    public void testGetBestTrackInfo_langAndChannelCountMatch() {
        TvTrackInfo result = getBestTrackInfo(ALL, UN_MATCHED_ID, "en", 5);
        assertEquals("best track ", INFO_2_EN_5, result);
    }

    public void testGetBestTrackInfo_languageOnlyMatch() {
        TvTrackInfo result = getBestTrackInfo(ALL, UN_MATCHED_ID, "fr", 1);
        assertEquals("best track ", INFO_3_FR_5, result);
    }

    public void testGetBestTrackInfo_noMatches() {
        TvTrackInfo result = getBestTrackInfo(ALL, UN_MATCHED_ID, "kr", 1);
        assertEquals("best track ", INFO_1_EN_1, result);
    }


    public void testComparator() {
        Comparator<TvTrackInfo> comparator = TvTrackInfoUtils.createComparator("1", "en", 1);
        ComparatorTester.withoutEqualsTest(comparator)
                // lang not match
                .addComparableGroup(create("1", "kr", 1), create("2", "kr", 2),
                        create("1", "ja", 1),
                        create("1", "ch", 1))
                 // lang match not count match
                .addComparableGroup(create("2", "en", 2), create("3", "en", 3),
                        create("1", "en", 2))
                 // lang and count match
                .addComparableGroup(create("2", "en", 1), create("3", "en", 1))
                 // all match
                .addComparableGroup(create("1", "en", 1), create("1", "en", 1))
                .test();
    }
}
