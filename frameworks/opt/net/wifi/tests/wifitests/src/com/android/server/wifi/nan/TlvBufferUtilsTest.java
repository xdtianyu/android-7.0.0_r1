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

package com.android.server.wifi.nan;

import static org.hamcrest.core.IsEqual.equalTo;

import android.net.wifi.nan.TlvBufferUtils;
import android.test.suitebuilder.annotation.SmallTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ErrorCollector;
import org.junit.rules.ExpectedException;

/**
 * Unit test harness for WifiNanManager class.
 */
@SmallTest
public class TlvBufferUtilsTest {
    @Rule
    public ErrorCollector collector = new ErrorCollector();

    @Rule
    public ExpectedException thrown = ExpectedException.none();

    /*
     * TlvBufferUtils Tests
     */

    @Test
    public void testTlvBuild() {
        TlvBufferUtils.TlvConstructor tlv11 = new TlvBufferUtils.TlvConstructor(1, 1);
        tlv11.allocate(15);
        tlv11.putByte(0, (byte) 2);
        tlv11.putByteArray(2, new byte[] {
                0, 1, 2 });

        collector.checkThat("tlv11-correct-construction",
                utilAreArraysEqual(tlv11.getArray(), tlv11.getActualLength(), new byte[] {
                        0, 1, 2, 2, 3, 0, 1, 2 }, 8),
                equalTo(true));

        TlvBufferUtils.TlvConstructor tlv01 = new TlvBufferUtils.TlvConstructor(0, 1);
        tlv01.allocate(15);
        tlv01.putByte(0, (byte) 2);
        tlv01.putByteArray(2, new byte[] {
                0, 1, 2 });

        collector.checkThat("tlv01-correct-construction",
                utilAreArraysEqual(tlv01.getArray(), tlv01.getActualLength(), new byte[] {
                        1, 2, 3, 0, 1, 2 }, 6),
                equalTo(true));
    }

    @Test
    public void testTlvIterate() {
        TlvBufferUtils.TlvConstructor tlv22 = new TlvBufferUtils.TlvConstructor(2, 2);
        tlv22.allocate(18);
        tlv22.putInt(0, 2);
        tlv22.putShort(2, (short) 3);
        tlv22.putZeroLengthElement(55);

        TlvBufferUtils.TlvIterable tlv22It = new TlvBufferUtils.TlvIterable(2, 2, tlv22.getArray(),
                tlv22.getActualLength());
        int count = 0;
        for (TlvBufferUtils.TlvElement tlv : tlv22It) {
            if (count == 0) {
                collector.checkThat("tlv22-correct-iteration-mType", tlv.mType, equalTo(0));
                collector.checkThat("tlv22-correct-iteration-mLength", tlv.mLength, equalTo(4));
                collector.checkThat("tlv22-correct-iteration-DATA", tlv.getInt(), equalTo(2));
            } else if (count == 1) {
                collector.checkThat("tlv22-correct-iteration-mType", tlv.mType, equalTo(2));
                collector.checkThat("tlv22-correct-iteration-mLength", tlv.mLength, equalTo(2));
                collector.checkThat("tlv22-correct-iteration-DATA", (int) tlv.getShort(),
                        equalTo(3));
            } else if (count == 2) {
                collector.checkThat("tlv22-correct-iteration-mType", tlv.mType, equalTo(55));
                collector.checkThat("tlv22-correct-iteration-mLength", tlv.mLength, equalTo(0));
            } else {
                collector.checkThat("Invalid number of iterations in loop - tlv22", true,
                        equalTo(false));
            }
            ++count;
        }
        if (count != 3) {
            collector.checkThat("Invalid number of iterations outside loop - tlv22", true,
                    equalTo(false));
        }

        TlvBufferUtils.TlvConstructor tlv02 = new TlvBufferUtils.TlvConstructor(0, 2);
        tlv02.allocate(15);
        tlv02.putByte(0, (byte) 2);
        tlv02.putString(0, "ABC");

        TlvBufferUtils.TlvIterable tlv02It = new TlvBufferUtils.TlvIterable(0, 2, tlv02.getArray(),
                tlv02.getActualLength());
        count = 0;
        for (TlvBufferUtils.TlvElement tlv : tlv02It) {
            if (count == 0) {
                collector.checkThat("tlv02-correct-iteration-mLength", tlv.mLength, equalTo(1));
                collector.checkThat("tlv02-correct-iteration-DATA", (int) tlv.getByte(),
                        equalTo(2));
            } else if (count == 1) {
                collector.checkThat("tlv02-correct-iteration-mLength", tlv.mLength, equalTo(3));
                collector.checkThat("tlv02-correct-iteration-DATA", tlv.getString().equals("ABC"),
                        equalTo(true));
            } else {
                collector.checkThat("Invalid number of iterations in loop - tlv02", true,
                        equalTo(false));
            }
            ++count;
        }
        if (count != 2) {
            collector.checkThat("Invalid number of iterations outside loop - tlv02", true,
                    equalTo(false));
        }
    }

    @Test
    public void testTlvInvalidSizeT1L0() {
        thrown.expect(IllegalArgumentException.class);
        TlvBufferUtils.TlvConstructor tlv10 = new TlvBufferUtils.TlvConstructor(1, 0);
    }

    @Test
    public void testTlvInvalidSizeTm3L2() {
        thrown.expect(IllegalArgumentException.class);
        TlvBufferUtils.TlvConstructor tlv10 = new TlvBufferUtils.TlvConstructor(-3, 2);
    }

    @Test
    public void testTlvInvalidSizeT1Lm2() {
        thrown.expect(IllegalArgumentException.class);
        TlvBufferUtils.TlvConstructor tlv10 = new TlvBufferUtils.TlvConstructor(1, -2);
    }

    @Test
    public void testTlvInvalidSizeT1L3() {
        thrown.expect(IllegalArgumentException.class);
        TlvBufferUtils.TlvConstructor tlv10 = new TlvBufferUtils.TlvConstructor(1, 3);
    }

    @Test
    public void testTlvInvalidSizeT3L1() {
        thrown.expect(IllegalArgumentException.class);
        TlvBufferUtils.TlvConstructor tlv10 = new TlvBufferUtils.TlvConstructor(3, 1);
    }

    @Test
    public void testTlvItInvalidSizeT1L0() {
        final byte[] dummy = {
                0, 1, 2 };
        final int dummyLength = 3;
        thrown.expect(IllegalArgumentException.class);
        TlvBufferUtils.TlvIterable tlvIt10 = new TlvBufferUtils.TlvIterable(1, 0, dummy,
                dummyLength);
    }

    @Test
    public void testTlvItInvalidSizeTm3L2() {
        final byte[] dummy = {
                0, 1, 2 };
        final int dummyLength = 3;
        thrown.expect(IllegalArgumentException.class);
        TlvBufferUtils.TlvIterable tlvIt10 = new TlvBufferUtils.TlvIterable(-3, 2, dummy,
                dummyLength);
    }

    @Test
    public void testTlvItInvalidSizeT1Lm2() {
        final byte[] dummy = {
                0, 1, 2 };
        final int dummyLength = 3;
        thrown.expect(IllegalArgumentException.class);
        TlvBufferUtils.TlvIterable tlvIt10 = new TlvBufferUtils.TlvIterable(1, -2, dummy,
                dummyLength);
    }

    @Test
    public void testTlvItInvalidSizeT1L3() {
        final byte[] dummy = {
                0, 1, 2 };
        final int dummyLength = 3;
        thrown.expect(IllegalArgumentException.class);
        TlvBufferUtils.TlvIterable tlvIt10 = new TlvBufferUtils.TlvIterable(1, 3, dummy,
                dummyLength);
    }

    @Test
    public void testTlvItInvalidSizeT3L1() {
        final byte[] dummy = {
                0, 1, 2 };
        final int dummyLength = 3;
        thrown.expect(IllegalArgumentException.class);
        TlvBufferUtils.TlvIterable tlvIt10 = new TlvBufferUtils.TlvIterable(3, 1, dummy,
                dummyLength);
    }

    /*
     * Utilities
     */

    private static boolean utilAreArraysEqual(byte[] x, int xLength, byte[] y, int yLength) {
        if (xLength != yLength) {
            return false;
        }

        if (x != null && y != null) {
            for (int i = 0; i < xLength; ++i) {
                if (x[i] != y[i]) {
                    return false;
                }
            }
        } else if (xLength != 0) {
            return false; // invalid != invalid
        }

        return true;
    }
}
