/*
 * Copyright (C) 2013 The Android Open Source Project
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

package org.junit;

//Note: this class was written without inspecting the junit code

import java.util.Arrays;

public class Assert extends junit.framework.Assert {
    protected Assert() {
    }

    public static void assertArrayEquals(byte[] expecteds, byte[] actuals) {
        assertArrayEquals("", expecteds, actuals);
    }

    public static void assertArrayEquals(String message, byte[] expecteds, byte[] actuals) {
        String expectedString = Arrays.toString(expecteds);
        String actualString = Arrays.toString(actuals);

        if (!expectedString.equals(actualString)) {
            fail(message, "expected " + expectedString + " but was " + actualString);
        }
    }

    public static void assertArrayEquals(char[] expecteds, char[] actuals) {
        assertArrayEquals("", expecteds, actuals);
    }

    public static void assertArrayEquals(String message, char[] expecteds, char[] actuals) {
        String expectedString = Arrays.toString(expecteds);
        String actualString = Arrays.toString(actuals);

        if (!expectedString.equals(actualString)) {
            fail(message, "expected " + expectedString + " but was " + actualString);
        }
    }

    public static void assertArrayEquals(int[] expecteds, int[] actuals) {
        assertArrayEquals("", expecteds, actuals);
    }

    public static void assertArrayEquals(String message, int[] expecteds, int[] actuals) {
        String expectedString = Arrays.toString(expecteds);
        String actualString = Arrays.toString(actuals);

        if (!expectedString.equals(actualString)) {
            fail(message, "expected " + expectedString + " but was " + actualString);
        }
    }

    public static void assertArrayEquals(long[] expecteds, long[] actuals) {
        assertArrayEquals("", expecteds, actuals);
    }

    public static void assertArrayEquals(String message, long[] expecteds, long[] actuals) {
        String expectedString = Arrays.toString(expecteds);
        String actualString = Arrays.toString(actuals);

        if (!expectedString.equals(actualString)) {
            fail(message, "expected " + expectedString + " but was " + actualString);
        }
    }

    public static void assertArrayEquals(Object[] expecteds, Object[] actuals) {
        assertArrayEquals("", expecteds, actuals);
    }

    public static void assertArrayEquals(String message, Object[] expecteds, Object[] actuals) {
        String expectedString = Arrays.toString(expecteds);
        String actualString = Arrays.toString(actuals);

        if (!expectedString.equals(actualString)) {
            fail(message, "expected " + expectedString + " but was " + actualString);
        }
    }

    public static void assertArrayEquals(short[] expecteds, short[] actuals) {
        assertArrayEquals("", expecteds, actuals);
    }

    public static void assertArrayEquals(String message, short[] expecteds, short[] actuals) {
        String expectedString = Arrays.toString(expecteds);
        String actualString = Arrays.toString(actuals);

        if (!expectedString.equals(actualString)) {
            fail(message, "expected " + expectedString + " but was " + actualString);
        }
    }
}
