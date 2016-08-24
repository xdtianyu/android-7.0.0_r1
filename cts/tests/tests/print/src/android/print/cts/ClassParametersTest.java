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

package android.print.cts;

import android.print.PrintAttributes;
import android.print.PrintAttributes.MediaSize;
import android.print.PrintAttributes.Resolution;
import android.print.PrintDocumentInfo;
import android.support.test.runner.AndroidJUnit4;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test that the print attributes are correctly propagated through the print framework
 */
@RunWith(AndroidJUnit4.class)
public class ClassParametersTest {
    /**
     * Run a runnable and expect and exception of a certain type.
     *
     * @param r The runnable to run
     * @param expectedClass The expected exception type
     */
    private void assertException(Runnable r, Class<? extends RuntimeException> expectedClass) {
        try {
            r.run();
        } catch (Exception e) {
            if (e.getClass().isAssignableFrom(expectedClass)) {
                return;
            } else {
                throw new AssertionError("Expected: " + expectedClass.getName() + ", got: "
                        + e.getClass().getName());
            }
        }

        throw new AssertionError("No exception thrown");
    }

    /**
     * Test that we cannot create PrintAttributes with illegal parameters.
     *
     * @throws Exception If anything is unexpected
     */
    @Test
    public void testIllegalPrintAttributes() throws Exception {
        assertException(() -> (new PrintAttributes.Builder()).setColorMode(-1),
                IllegalArgumentException.class);
        assertException(() -> (new PrintAttributes.Builder()).setColorMode(0),
                IllegalArgumentException.class);
        assertException(() -> (new PrintAttributes.Builder()).setColorMode(
                PrintAttributes.COLOR_MODE_COLOR | PrintAttributes.COLOR_MODE_MONOCHROME),
                IllegalArgumentException.class);

        assertException(() -> (new PrintAttributes.Builder()).setDuplexMode(-1),
                IllegalArgumentException.class);
        assertException(() -> (new PrintAttributes.Builder()).setDuplexMode(0),
                IllegalArgumentException.class);
        assertException(() -> (new PrintAttributes.Builder()).setDuplexMode(
                PrintAttributes.DUPLEX_MODE_LONG_EDGE | PrintAttributes.DUPLEX_MODE_NONE),
                IllegalArgumentException.class);

        assertException(() -> new Resolution(null, "label", 10, 10),
                IllegalArgumentException.class);
        assertException(() -> new Resolution("", "label", 10, 10),
                IllegalArgumentException.class);
        assertException(() -> new Resolution("id", null, 10, 10),
                IllegalArgumentException.class);
        assertException(() -> new Resolution("id", "", 10, 10),
                IllegalArgumentException.class);
        assertException(() -> new Resolution("id", "label", -10, 10),
                IllegalArgumentException.class);
        assertException(() -> new Resolution("id", "label", 0, 10),
                IllegalArgumentException.class);
        assertException(() -> new Resolution("id", "label", 10, -10),
                IllegalArgumentException.class);
        assertException(() -> new Resolution("id", "label", 10, 0),
                IllegalArgumentException.class);

        assertException(() -> new MediaSize(null, "label", 10, 10),
                IllegalArgumentException.class);
        assertException(() -> new MediaSize("", "label", 10, 10),
                IllegalArgumentException.class);
        assertException(() -> new MediaSize("id", null, 10, 10),
                IllegalArgumentException.class);
        assertException(() -> new MediaSize("id", "", 10, 10),
                IllegalArgumentException.class);
        assertException(() -> new MediaSize("id", "label", -10, 10),
                IllegalArgumentException.class);
        assertException(() -> new MediaSize("id", "label", 0, 10),
                IllegalArgumentException.class);
        assertException(() -> new MediaSize("id", "label", 10, -10),
                IllegalArgumentException.class);
        assertException(() -> new MediaSize("id", "label", 10, 0),
                IllegalArgumentException.class);

        // There is no restrictions on what parameters to set for minMargins.
    }

    /**
     * Test that we cannot create PrintDocumentInfo with illegal parameters.
     *
     * @throws Exception If anything is unexpected
     */
    @Test
    public void testIllegalPrintDocumentInfo() throws Exception {
        assertException(() -> new PrintDocumentInfo.Builder(null),
                IllegalArgumentException.class);
        assertException(() -> new PrintDocumentInfo.Builder(""),
                IllegalArgumentException.class);

        assertException(() -> new PrintDocumentInfo.Builder("doc").setPageCount(-2),
                IllegalArgumentException.class);
        // -1 == UNKNOWN and 0 are allowed

        // Content type is not restricted
    }
}
