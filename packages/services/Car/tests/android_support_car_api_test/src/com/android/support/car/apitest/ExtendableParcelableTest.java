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
package com.android.support.car.apitest;

import android.os.Parcel;
import android.support.car.annotation.VersionDef;
import android.support.car.os.ExtendableParcelable;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Log;

import java.util.Arrays;

@SmallTest
public class ExtendableParcelableTest extends AndroidTestCase {

    private static final String TAG = ExtendableParcelableTest.class.getSimpleName();

    public static class V1Parcelable extends ExtendableParcelable {
        @VersionDef(version = 1)
        public final byte[] byteData0;
        @VersionDef(version = 1)
        public final int intData0;
        @VersionDef(version = 1)
        public final String stringData0;
        @VersionDef(version = 1)
        public final int intData1;

        private static final int VERSION = 1;

        public V1Parcelable(byte[] byteData0, int intData0, String stringData0, int intData1) {
            super(VERSION);
            this.byteData0 = byteData0;
            this.intData0 = intData0;
            this.stringData0 = stringData0;
            this.intData1 = intData1;
        }

        public V1Parcelable(Parcel in) {
            super(in, VERSION);
            int lastPosition = readHeader(in);
            byteData0 = in.createByteArray();
            intData0 = in.readInt();
            stringData0 = in.readString();
            intData1 = in.readInt();
            completeReading(in, lastPosition);
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            int pos = writeHeader(dest);
            dest.writeByteArray(byteData0);
            dest.writeInt(intData0);
            dest.writeString(stringData0);
            dest.writeInt(intData1);
            completeWriting(dest, pos);
        }

        @Override
        public boolean equals(Object o) {
            if (o instanceof V1Parcelable) {
                V1Parcelable other = (V1Parcelable) o;
                return Arrays.equals(byteData0, other.byteData0) && intData0 == other.intData0 &&
                        (stringData0 == null ? other.stringData0 == null :
                            stringData0.equals(other.stringData0)) &&
                            intData1 == other.intData1;
            }
            return false;
        }

        @Override
        public String toString() {
            return "V1Parcelable byteData0:" + Arrays.toString(byteData0) + ",intData0:" + intData0
                    + ",stringData0:" + stringData0 + ",intData1:" + intData1;
        }
    }

    public static class V2Parcelable extends ExtendableParcelable {
        @VersionDef(version = 1)
        public final byte[] byteData0;
        @VersionDef(version = 1)
        public final int intData0;
        @VersionDef(version = 1)
        public final String stringData0;
        @VersionDef(version = 1)
        public final int intData1;
        @VersionDef(version = 2)
        public final byte[] byteData1;
        @VersionDef(version = 2)
        public final int intData2;

        private static final int VERSION = 2;

        public V2Parcelable(byte[] byteData0, int intData0, String stringData0, int intData1,
                byte[] byteData1, int intData2) {
            super(VERSION);
            this.byteData0 = byteData0;
            this.intData0 = intData0;
            this.stringData0 = stringData0;
            this.intData1 = intData1;
            this.byteData1 = byteData1;
            this.intData2 = intData2;
        }

        public V2Parcelable(Parcel in) {
            super(in, VERSION);
            int lastPosition = readHeader(in);
            byteData0 = in.createByteArray();
            intData0 = in.readInt();
            stringData0 = in.readString();
            intData1 = in.readInt();
            if (version >= 2) { // do not use VERSION here as VERSION will become 3 in next revision
                byteData1 = in.createByteArray();
                intData2 = in.readInt();
            } else {
                byteData1 = null;
                intData2 = 0;
            }
            completeReading(in, lastPosition);
        }

        /** provide has method if null check is not possible. */
        public boolean hasIntData1() {
            return version >= 2;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            int pos = writeHeader(dest);
            dest.writeByteArray(byteData0);
            dest.writeInt(intData0);
            dest.writeString(stringData0);
            dest.writeInt(intData1);
            dest.writeByteArray(byteData1);
            dest.writeInt(intData2);
            completeWriting(dest, pos);
        }

        @Override
        public boolean equals(Object o) {
            if (o instanceof V2Parcelable) {
                V2Parcelable other = (V2Parcelable) o;
                return Arrays.equals(byteData0, other.byteData0) && intData0 == other.intData0 &&
                        (stringData0 == null ? other.stringData0 == null :
                            stringData0.equals(other.stringData0)) &&
                            intData1 == other.intData1 && Arrays.equals(byteData1, other.byteData1)
                            && intData2 == other.intData2;
            }
            return false;
        }

        @Override
        public String toString() {
            return "V2Parcelable byteData0:" + Arrays.toString(byteData0) + ",intData0:" + intData0
                    + ",stringData0:" + stringData0 + ",intData1:" + intData1 +
                    ",byteData1:" + Arrays.toString(byteData1) + ",intData2:" + intData2;
        }
    }

    public static class V3Parcelable extends ExtendableParcelable {
        @VersionDef(version = 1)
        public final byte[] byteData0;
        @VersionDef(version = 1)
        public final int intData0;
        @VersionDef(version = 1)
        public final String stringData0;
        @VersionDef(version = 1)
        public final int intData1;
        @VersionDef(version = 2)
        public final byte[] byteData1;
        @VersionDef(version = 2)
        public final int intData2;
        @VersionDef(version = 3)
        public final String stringData1;
        @VersionDef(version = 3)
        public final int intData3;

        private static final int VERSION = 3;

        public V3Parcelable(byte[] byteData0, int intData0, String stringData0, int intData1,
                byte[] byteData1, int intData2, String stringData1, int intData3) {
            super(VERSION);
            this.byteData0 = byteData0;
            this.intData0 = intData0;
            this.stringData0 = stringData0;
            this.intData1 = intData1;
            this.byteData1 = byteData1;
            this.intData2 = intData2;
            this.stringData1 = stringData1;
            this.intData3 = intData3;
        }

        public V3Parcelable(Parcel in) {
            super(in, VERSION);
            int lastPosition = readHeader(in);
            byteData0 = in.createByteArray();
            intData0 = in.readInt();
            stringData0 = in.readString();
            intData1 = in.readInt();
            if (version >= 2) {
                byteData1 = in.createByteArray();
                intData2 = in.readInt();
            } else {
                byteData1 = null;
                intData2 = 0;
            }
            if (version >= 3) {
                stringData1 = in.readString();
                intData3 = in.readInt();
            } else {
                stringData1 = null;
                intData3 = 0;
            }
            completeReading(in, lastPosition);
        }

        /** provide has method if null check is not possible. */
        public boolean hasIntData1() {
            return version >= 2;
        }

        /** provide has method if null check is not possible. */
        public boolean hasIntData2() {
            return version >= 3;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            int pos = writeHeader(dest);
            dest.writeByteArray(byteData0);
            dest.writeInt(intData0);
            dest.writeString(stringData0);
            dest.writeInt(intData1);
            dest.writeByteArray(byteData1);
            dest.writeInt(intData2);
            dest.writeString(stringData1);
            dest.writeInt(intData3);
            completeWriting(dest, pos);
        }

        @Override
        public boolean equals(Object o) {
            if (o instanceof V3Parcelable) {
                V3Parcelable other = (V3Parcelable) o;
                return Arrays.equals(byteData0, other.byteData0) && intData0 == other.intData0 &&
                        (stringData0 == null ? other.stringData0 == null :
                            stringData0.equals(other.stringData0)) &&
                            intData1 == other.intData1 && Arrays.equals(byteData1, other.byteData1)
                            && intData2 == other.intData2 &&
                            (stringData1 == null ? other.stringData1 == null :
                                stringData1.equals(other.stringData1)) &&
                            intData3 == other.intData3;
            }
            return false;
        }

        @Override
        public String toString() {
            return "V3Parcelable byteData0:" + Arrays.toString(byteData0) + ",intData0:" + intData0
                    + ",stringData0:" + stringData0 + ",intData1:" + intData1 +
                    ",byteData1:" + Arrays.toString(byteData1) + ",intData2:" + intData2 +
                    ",stringData1:" + stringData1 + ",intData3:" + intData3;
        }
    }

    public void testV1ToV3() throws Exception {
        Parcel p = Parcel.obtain();
        int startPos = p.dataPosition();
        byte[] byteData0 = new byte[] { 0x0, 0x1, 0x2 };
        int intData0 = 1234;
        String stringData0 = null;
        int intData1 = 5678;
        V1Parcelable v1 = new V1Parcelable(byteData0, intData0, stringData0, intData1);
        // expected after reading parcel
        V3Parcelable v3Expected = new V3Parcelable(byteData0, intData0, stringData0, intData1,
                null, 0, null, 0);
        v1.writeToParcel(p, 0);
        final int additionalData = 0x8fffffff;
        p.writeInt(additionalData);

        p.setDataPosition(startPos);
        V3Parcelable v3 = new V3Parcelable(p);
        Log.i(TAG, "v1:" + v1);
        Log.i(TAG, "v3 expected:" + v3Expected);
        Log.i(TAG, "v3 read:" + v3);
        assertTrue(v3Expected.equals(v3));
        assertEquals(1, v3.version);
        assertFalse(v3.hasIntData1());
        assertFalse(v3.hasIntData2());
        assertEquals(additionalData, p.readInt());
    }

    public void testV2ToV3() throws Exception {
        Parcel p = Parcel.obtain();
        int startPos = p.dataPosition();
        byte[] byteData0 = new byte[] { 0x0, 0x1, 0x2 };
        int intData0 = 1234;
        String stringData0 = null;
        int intData1 = 5678;
        byte[] byteData1 = new byte[] { 0x3, 0x4, 0x5 };
        int intData2 = 9012;
        V2Parcelable v2 = new V2Parcelable(byteData0, intData0, stringData0, intData1, byteData1,
                intData2);
        // expected after reading parcel
        V3Parcelable v3Expected = new V3Parcelable(byteData0, intData0, stringData0, intData1,
                byteData1, intData2, null, 0);
        v2.writeToParcel(p, 0);
        final int additionalData = 0x8fffffff;
        p.writeInt(additionalData);

        p.setDataPosition(startPos);
        V3Parcelable v3 = new V3Parcelable(p);
        Log.i(TAG, "v2:" + v2);
        Log.i(TAG, "v3 expected:" + v3Expected);
        Log.i(TAG, "v3 read:" + v3);
        assertTrue(v3Expected.equals(v3));
        assertEquals(2, v3.version);
        assertTrue(v3.hasIntData1());
        assertFalse(v3.hasIntData2());
        assertEquals(additionalData, p.readInt());
    }

    public void testV3ToV1() throws Exception {
        Parcel p = Parcel.obtain();
        int startPos = p.dataPosition();
        byte[] byteData0 = new byte[] { 0x0, 0x1, 0x2 };
        int intData0 = 1234;
        String stringData0 = null;
        int intData1 = 5678;
        byte[] byteData1 = new byte[] { 0x3, 0x4, 0x5 };
        int intData2 = 9012;
        // v3
        String stringData1 = "Hello";
        int intData3 = -1;
        V3Parcelable v3 = new V3Parcelable(byteData0, intData0, stringData0, intData1, byteData1,
                intData2, stringData1, intData3);
        // expected after reading parcel
        V1Parcelable v1Expected = new V1Parcelable(byteData0, intData0, stringData0, intData1);
        v3.writeToParcel(p, 0);
        final int additionalData = 0x8fffffff;
        p.writeInt(additionalData);

        p.setDataPosition(startPos);
        V1Parcelable v1 = new V1Parcelable(p);
        Log.i(TAG, "v3:" + v3);
        Log.i(TAG, "v1 expected:" + v1Expected);
        Log.i(TAG, "v1 read:" + v1);
        assertTrue(v1Expected.equals(v1));
        assertEquals(1, v1.version);
        assertEquals(additionalData, p.readInt());
    }

    public void testV2ToV2() throws Exception {
        Parcel p = Parcel.obtain();
        int startPos = p.dataPosition();
        byte[] byteData0 = new byte[] { 0x0, 0x1, 0x2 };
        int intData0 = 1234;
        String stringData0 = null;
        int intData1 = 5678;
        byte[] byteData1 = new byte[] { 0x3, 0x4, 0x5 };
        int intData2 = 9012;
        V2Parcelable v2 = new V2Parcelable(byteData0, intData0, stringData0, intData1, byteData1,
                intData2);
        v2.writeToParcel(p, 0);
        final int additionalData = 0x8fffffff;
        p.writeInt(additionalData);

        p.setDataPosition(startPos);
        V2Parcelable v2Read = new V2Parcelable(p);
        Log.i(TAG, "v2:" + v2);
        Log.i(TAG, "v2 read:" + v2Read);
        assertTrue(v2.equals(v2Read));
        assertEquals(2, v2.version);
        assertTrue(v2.hasIntData1());
        assertEquals(additionalData, p.readInt());
    }

    public void testV3ToV1Array() throws Exception {
        Parcel p = Parcel.obtain();
        int startPos = p.dataPosition();
        byte[] byteData0 = new byte[] { 0x0, 0x1, 0x2 };
        int intData0 = 1234;
        String stringData0 = null;
        int intData1 = 5678;
        byte[] byteData1 = new byte[] { 0x3, 0x4, 0x5 };
        int intData2 = 9012;
        // v3
        String stringData1 = "Hello";
        int intData3 = -1;
        V3Parcelable v3_0 = new V3Parcelable(byteData0, intData0, stringData0, intData1, byteData1,
                intData2, stringData1, intData3);
        V1Parcelable v1Expected0 = new V1Parcelable(byteData0, intData0, stringData0, intData1);
        byteData0 = null;
        intData0 = 1;
        stringData0 = "world";
        V3Parcelable v3_1 = new V3Parcelable(byteData0, intData0, stringData0, intData1, byteData1,
                intData2, stringData1, intData3);
        V1Parcelable v1Expected1 = new V1Parcelable(byteData0, intData0, stringData0, intData1);
        // test write of arrays without initial length portion
        v3_0.writeToParcel(p, 0);
        v3_1.writeToParcel(p, 0);
        final int additionalData = 0x8fffffff;
        p.writeInt(additionalData);

        p.setDataPosition(startPos);
        V1Parcelable v1_0 = new V1Parcelable(p);
        V1Parcelable v1_1 = new V1Parcelable(p);
        Log.i(TAG, "v3_1:" + v3_1);
        Log.i(TAG, "v1 expected 1:" + v1Expected1);
        Log.i(TAG, "v1_1 read:" + v1_1);
        assertTrue(v1Expected0.equals(v1_0));
        assertTrue(v1Expected1.equals(v1_1));
        assertEquals(1, v1_0.version);
        assertEquals(1, v1_1.version);
        assertEquals(additionalData, p.readInt());
    }

    public void testV1ToV3Array() throws Exception {
        Parcel p = Parcel.obtain();
        int startPos = p.dataPosition();
        byte[] byteData0 = new byte[] { 0x0, 0x1, 0x2 };
        int intData0 = 1234;
        String stringData0 = null;
        int intData1 = 5678;
        V1Parcelable v1_0 = new V1Parcelable(byteData0, intData0, stringData0, intData1);
        // expected after reading parcel
        V3Parcelable v3Expected0 = new V3Parcelable(byteData0, intData0, stringData0, intData1,
                null, 0, null, 0);
        byteData0 = null;
        intData0 = 4567;
        stringData0 = "Hi";
        V1Parcelable v1_1 = new V1Parcelable(byteData0, intData0, stringData0, intData1);
        // expected after reading parcel
        V3Parcelable v3Expected1 = new V3Parcelable(byteData0, intData0, stringData0, intData1,
                null, 0, null, 0);
        v1_0.writeToParcel(p, 0);
        v1_1.writeToParcel(p, 0);
        final int additionalData = 0x8fffffff;
        p.writeInt(additionalData);

        p.setDataPosition(startPos);
        V3Parcelable v3_0 = new V3Parcelable(p);
        V3Parcelable v3_1 = new V3Parcelable(p);
        Log.i(TAG, "v1:" + v1_1);
        Log.i(TAG, "v3 expected:" + v3Expected1);
        Log.i(TAG, "v3 read:" + v3_1);
        assertTrue(v3Expected0.equals(v3_0));
        assertFalse(v3_0.hasIntData1());
        assertFalse(v3_0.hasIntData2());
        assertFalse(v3_1.hasIntData1());
        assertFalse(v3_1.hasIntData2());
        assertEquals(1, v3_0.version);
        assertEquals(1, v3_1.version);
        assertEquals(additionalData, p.readInt());
    }
}
