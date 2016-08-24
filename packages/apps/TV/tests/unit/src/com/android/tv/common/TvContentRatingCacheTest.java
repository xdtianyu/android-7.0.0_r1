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

package com.android.tv.common;

import android.content.ComponentCallbacks2;
import android.media.tv.TvContentRating;
import android.test.AndroidTestCase;
import android.test.MoreAsserts;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.tv.testing.TvContentRatingConstants;
import com.android.tv.util.Utils;

/**
 * Test for {@link android.media.tv.TvContentRating} tests in {@link Utils}.
 */
@SmallTest
public class TvContentRatingCacheTest extends AndroidTestCase {

    /**
     * US_TV_MA and US_TV_Y7 in order
     */
    public static final String MA_AND_Y7 = TvContentRatingConstants.STRING_US_TV_MA + ","
            + TvContentRatingConstants.STRING_US_TV_Y7_US_TV_FV;

    /**
     * US_TV_MA and US_TV_Y7 not in order
     */
    public static final String Y7_AND_MA = TvContentRatingConstants.STRING_US_TV_Y7_US_TV_FV + ","
            + TvContentRatingConstants.STRING_US_TV_MA;
    TvContentRatingCache mCache = TvContentRatingCache.getInstance();

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mCache.performTrimMemory(ComponentCallbacks2.TRIM_MEMORY_COMPLETE);
    }

    @Override
    protected void tearDown() throws Exception {
        mCache.performTrimMemory(ComponentCallbacks2.TRIM_MEMORY_COMPLETE);
        super.tearDown();
    }

    public void testGetRatings_US_TV_MA() {
        TvContentRating[] result = mCache.getRatings(TvContentRatingConstants.STRING_US_TV_MA);
        MoreAsserts.assertEquals(asArray(TvContentRatingConstants.CONTENT_RATING_US_TV_MA), result);
    }

    public void testGetRatings_US_TV_MA_same() {
        TvContentRating[] first = mCache.getRatings(TvContentRatingConstants.STRING_US_TV_MA);
        TvContentRating[] second = mCache.getRatings(TvContentRatingConstants.STRING_US_TV_MA);
        assertSame(first, second);
    }

    public void testGetRatings_US_TV_MA_diffAfterClear() {
        TvContentRating[] first = mCache.getRatings(TvContentRatingConstants.STRING_US_TV_MA);
        mCache.performTrimMemory(ComponentCallbacks2.TRIM_MEMORY_COMPLETE);
        TvContentRating[] second = mCache.getRatings(TvContentRatingConstants.STRING_US_TV_MA);
        assertNotSame(first, second);
    }

    public void testGetRatings_TWO_orderDoesNotMatter() {
        TvContentRating[] first = mCache.getRatings(MA_AND_Y7);
        TvContentRating[] second = mCache.getRatings(Y7_AND_MA);
        assertSame(first, second);
    }

    public void testContentRatingsToString_null() {
        String result = TvContentRatingCache.contentRatingsToString(null);
        assertEquals("ratings string", null, result);
    }

    public void testContentRatingsToString_none() {
        String result = TvContentRatingCache.contentRatingsToString(asArray());
        assertEquals("ratings string", null, result);
    }

    public void testContentRatingsToString_one() {
        String result = TvContentRatingCache
                .contentRatingsToString(asArray(TvContentRatingConstants.CONTENT_RATING_US_TV_MA));
        assertEquals("ratings string", TvContentRatingConstants.STRING_US_TV_MA, result);
    }

    public void testContentRatingsToString_twoInOrder() {
        String result = TvContentRatingCache.contentRatingsToString(
                asArray(TvContentRatingConstants.CONTENT_RATING_US_TV_MA,
                        TvContentRatingConstants.CONTENT_RATING_US_TV_Y7_US_TV_FV));
        assertEquals("ratings string", MA_AND_Y7, result);
    }

    public void testContentRatingsToString_twoNotInOrder() {
        String result = TvContentRatingCache.contentRatingsToString(asArray(
                TvContentRatingConstants.CONTENT_RATING_US_TV_Y7_US_TV_FV,
                TvContentRatingConstants.CONTENT_RATING_US_TV_MA));
        assertEquals("ratings string", MA_AND_Y7, result);
    }

    public void testContentRatingsToString_double() {
        String result = TvContentRatingCache.contentRatingsToString(asArray(
                TvContentRatingConstants.CONTENT_RATING_US_TV_MA,
                TvContentRatingConstants.CONTENT_RATING_US_TV_MA));
        assertEquals("ratings string", TvContentRatingConstants.STRING_US_TV_MA, result);
    }

    public void testStringToContentRatings_null() {
        assertNull(TvContentRatingCache.stringToContentRatings(null));
    }

    public void testStringToContentRatings_none() {
        assertNull(TvContentRatingCache.stringToContentRatings(""));
    }

    public void testStringToContentRatings_bad() {
        assertNull(TvContentRatingCache.stringToContentRatings("bad"));
    }

    public void testStringToContentRatings_oneGoodOneBad() {
        TvContentRating[] results = TvContentRatingCache
                .stringToContentRatings(TvContentRatingConstants.STRING_US_TV_Y7_US_TV_FV + ",bad");
        MoreAsserts.assertEquals("ratings",
                asArray(TvContentRatingConstants.CONTENT_RATING_US_TV_Y7_US_TV_FV), results);
    }

    public void testStringToContentRatings_one() {
        TvContentRating[] results = TvContentRatingCache
                .stringToContentRatings(TvContentRatingConstants.STRING_US_TV_Y7_US_TV_FV);
        MoreAsserts.assertEquals("ratings",
                asArray(TvContentRatingConstants.CONTENT_RATING_US_TV_Y7_US_TV_FV), results);
    }

    public void testStringToContentRatings_twoNotInOrder() {
        TvContentRating[] results = TvContentRatingCache.stringToContentRatings(Y7_AND_MA);
        MoreAsserts.assertEquals("ratings",
                asArray(TvContentRatingConstants.CONTENT_RATING_US_TV_MA,
                        TvContentRatingConstants.CONTENT_RATING_US_TV_Y7_US_TV_FV), results);
    }

    public void testStringToContentRatings_twoInOrder() {
        TvContentRating[] results = TvContentRatingCache.stringToContentRatings(MA_AND_Y7);
        MoreAsserts.assertEquals("ratings",
                asArray(TvContentRatingConstants.CONTENT_RATING_US_TV_MA,
                        TvContentRatingConstants.CONTENT_RATING_US_TV_Y7_US_TV_FV), results);
    }

    public void testStringToContentRatings_double() {
        TvContentRating[] results = TvContentRatingCache.stringToContentRatings(
                TvContentRatingConstants.STRING_US_TV_MA + ","
                        + TvContentRatingConstants.STRING_US_TV_MA);
        MoreAsserts
                .assertEquals("ratings", asArray(TvContentRatingConstants.CONTENT_RATING_US_TV_MA),
                        results);
    }

    private static TvContentRating[] asArray(TvContentRating... ratings) {
        return ratings;
    }
}
