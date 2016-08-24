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
 * limitations under the License.
 */
package android.icu.cts.coverage.text;

import android.icu.text.AlphabeticIndex;
import android.icu.util.ULocale;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Locale;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

@RunWith(JUnit4.class)
public class AlphabeticIndexTest {
    @Test
    public void testAddLabels_Locale() {
        AlphabeticIndex<?> ulocaleIndex = new AlphabeticIndex<>(ULocale.CANADA);
        AlphabeticIndex<?> localeIndex = new AlphabeticIndex<>(Locale.CANADA);

        ulocaleIndex.addLabels(ULocale.SIMPLIFIED_CHINESE);
        localeIndex.addLabels(Locale.SIMPLIFIED_CHINESE);

        assertEquals(ulocaleIndex.getBucketLabels(), localeIndex.getBucketLabels());
    }

    @Test
    public void testGetRecordCount_empty() {
        assertEquals(0, new AlphabeticIndex<>(ULocale.CANADA).getRecordCount());
    }

    @Test
    public void testGetRecordCount_withRecords() {
        assertEquals(1, new AlphabeticIndex<>(ULocale.CANADA).addRecord("foo", null)
                .getRecordCount());
    }

    /**
     * Check that setUnderflowLabel/setOverflowLabel/setInflowLabel correctly influence the name of
     * generated labels.
     */
    @Test
    public void testFlowLabels() {
        AlphabeticIndex<?> index = new AlphabeticIndex<>(ULocale.ENGLISH)
                .addLabels(ULocale.forLanguageTag("ru"));
        index.setUnderflowLabel("underflow");
        index.setOverflowLabel("overflow");
        index.setInflowLabel("inflow");
        index.addRecord("!", null);
        index.addRecord("\u03B1", null); // GREEK SMALL LETTER ALPHA
        index.addRecord("\uab70", null); // CHEROKEE SMALL LETTER A


        AlphabeticIndex.Bucket<?> underflowBucket = null;
        AlphabeticIndex.Bucket<?> overflowBucket = null;
        AlphabeticIndex.Bucket<?> inflowBucket = null;
        for (AlphabeticIndex.Bucket<?> bucket : index) {
            switch (bucket.getLabelType()) {
                case UNDERFLOW:
                    assertNull(underflowBucket);
                    underflowBucket = bucket;
                    break;
                case OVERFLOW:
                    assertNull(overflowBucket);
                    overflowBucket = bucket;
                    break;
                case INFLOW:
                    assertNull(inflowBucket);
                    inflowBucket = bucket;
                    break;
            }
        }
        assertNotNull(underflowBucket);
        assertEquals("underflow", underflowBucket.getLabel());
        assertEquals("underflow", index.getUnderflowLabel());
        assertEquals(1, underflowBucket.size());

        assertNotNull(overflowBucket);
        assertEquals("overflow", overflowBucket.getLabel());
        assertEquals("overflow", index.getOverflowLabel());
        assertEquals(1, overflowBucket.size());

        assertNotNull(inflowBucket);
        assertEquals("inflow", inflowBucket.getLabel());
        assertEquals("inflow", index.getInflowLabel());
        assertEquals(1, inflowBucket.size());
    }
}
