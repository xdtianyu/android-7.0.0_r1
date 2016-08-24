/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.bluetooth.util;

import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

/**
 * Tests for {@link NumberUtils}.
 */
public class NumberUtilsTest extends AndroidTestCase {

    @SmallTest
    public static void testUnsignedByteToInt() {
        assertEquals(0, NumberUtils.unsignedByteToInt((byte) 0));
        assertEquals(19, NumberUtils.unsignedByteToInt((byte) 19));
        assertEquals(154, NumberUtils.unsignedByteToInt((byte) 154));
    }

    @SmallTest
    public void testLittleEndianByteArrayToInt() {
        assertEquals(1, NumberUtils.littleEndianByteArrayToInt(new byte[] {
                1 }));
        assertEquals(513, NumberUtils.littleEndianByteArrayToInt(new byte[] {
                1, 2 }));
        assertEquals(197121, NumberUtils.littleEndianByteArrayToInt(new byte[] {
                1, 2, 3 }));
    }
}
