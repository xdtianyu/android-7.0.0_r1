
package com.android.bluetooth.tests;

import android.test.AndroidTestCase;
import android.util.Log;
import android.util.Base64;

import java.io.UnsupportedEncodingException;
import java.text.SimpleDateFormat;
import java.util.Date;

import com.android.bluetooth.SignedLongLong;
import com.android.bluetooth.map.BluetoothMapUtils;

public class BluetoothMapUtilsTest extends AndroidTestCase {
    private static final String TAG = "BluetoothMapUtilsTest";

    private static final boolean D = true;
    private static final boolean V = true;
    private static final String encText1 = "=?UTF-8?b?w6bDuMOlw4bDmMOF?="; //æøåÆØÅ base64
    private static final String encText2 = "=?UTF-8?B?w6bDuMOlw4bDmMOF?="; //æøåÆØÅ base64
    private static final String encText3 = "=?UTF-8?q?=C3=A6=C3=B8=C3=A5=C3=86=C3=98=C3=85?="; //æøåÆØÅ QP
    private static final String encText4 = "=?UTF-8?Q?=C3=A6=C3=B8=C3=A5=C3=86=C3=98=C3=85?="; //æøåÆØÅ QP
    private static final String encText5 = "=?UTF-8?B?=C3=A6=C3=B8=C3=A5=C3=86=C3=98=C3=85?="; //QP in base64 string - should not compute
    private static final String encText6 = "=?UTF-8?Q?w6bDuMOlw4bDmMOF?="; //æøåÆØÅ base64 in QP stirng - should not compute
    private static final String encText7 = "this is a split =?UTF-8?Q?=C3=A6=C3=B8=C3=A5 ###123?= with more =?UTF-8?Q?=C3=A6=C3=B8=C3=A5 ###123?= inside"; // mix of QP and normal
    private static final String encText8 = "this is a split =?UTF-8?B?w6bDuMOlICMjIzEyMw==?= with more =?UTF-8?Q?=C3=A6=C3=B8=C3=A5 ###123?= inside"; // mix of normal, QP and Base64
    private static final String encText9 = "=?UTF-8?Q??=";
    private static final String encText10 = "=?UTF-8?Q??=";
    private static final String encText11 = "=?UTF-8?Q??=";

    private static final String decText1 = "æøåÆØÅ";
    private static final String decText2 = "æøåÆØÅ";
    private static final String decText3 = "æøåÆØÅ";
    private static final String decText4 = "æøåÆØÅ";
    private static final String decText5 = encText5;
    private static final String decText6 = "w6bDuMOlw4bDmMOF";
    private static final String decText7 = "this is a split æøå ###123 with more æøå ###123 inside";
    private static final String decText8 = "this is a split æøå ###123 with more æøå ###123 inside";

    public BluetoothMapUtilsTest() {
        super();

    }


    public void testEncoder(){
        assertTrue(BluetoothMapUtils.stripEncoding(encText1).equals(decText1));
        assertTrue(BluetoothMapUtils.stripEncoding(encText2).equals(decText2));
        assertTrue(BluetoothMapUtils.stripEncoding(encText3).equals(decText3));
        assertTrue(BluetoothMapUtils.stripEncoding(encText4).equals(decText4));
        assertTrue(BluetoothMapUtils.stripEncoding(encText5).equals(decText5));
        assertTrue(BluetoothMapUtils.stripEncoding(encText6).equals(decText6));
        Log.i(TAG,"##############################enc7:" +
                BluetoothMapUtils.stripEncoding(encText7));
        assertTrue(BluetoothMapUtils.stripEncoding(encText7).equals(decText7));
        assertTrue(BluetoothMapUtils.stripEncoding(encText8).equals(decText8));
    }

    public void testXBtUid() throws UnsupportedEncodingException {
        {
            SignedLongLong expected = new SignedLongLong(0x12345678L, 0x90abcdefL);
            /* this will cause an exception, since the value is too big... */
            SignedLongLong value;
            value = SignedLongLong.fromString("90abcdef0000000012345678");
            assertTrue("expected: " + expected + " value = " + value,
                    0 == value.compareTo(expected));
            assertEquals("expected: " + expected + " value = " + value,
                    expected.toHexString(), value.toHexString());
            Log.i(TAG,"Succesfully compared : " + value);
        }
        {
            SignedLongLong expected = new SignedLongLong(0x12345678L, 0xfedcba9890abcdefL);
            /* this will cause an exception, since the value is too big... */
            SignedLongLong value;
            value = SignedLongLong.fromString("fedcba9890abcdef0000000012345678");
            assertTrue("expected: " + expected + " value = " + value,
                    0 == value.compareTo(expected));
            assertEquals("expected: " + expected + " value = " + value,
                    expected.toHexString(), value.toHexString());
            Log.i(TAG,"Succesfully compared : " + value);
        }
        {
            SignedLongLong expected = new SignedLongLong(0x12345678L, 0);
            SignedLongLong value = SignedLongLong.fromString("000012345678");
            assertTrue("expected: " + expected + " value = " + value,
                    0 == value.compareTo(expected));
            assertEquals("expected: " + expected + " value = " + value,
                    expected.toHexString(), value.toHexString());
            Log.i(TAG,"Succesfully compared : " + value);
        }
        {
            SignedLongLong expected = new SignedLongLong(0x12345678L, 0);
            SignedLongLong value = SignedLongLong.fromString("12345678");
            assertTrue("expected: " + expected + " value = " + value,
                    0 == value.compareTo(expected));
            assertEquals("expected: " + expected + " value = " + value,
                    expected.toHexString(), value.toHexString());
            Log.i(TAG,"Succesfully compared : " + value);
        }
        {
            SignedLongLong expected = new SignedLongLong(0x123456789abcdef1L, 0x9L);
            SignedLongLong value = SignedLongLong.fromString("0009123456789abcdef1");
            assertTrue("expected: " + expected + " value = " + value,
                    0 == value.compareTo(expected));
            assertEquals("expected: " + expected + " value = " + value,
                    expected.toHexString(), value.toHexString());
            Log.i(TAG,"Succesfully compared : " + value);
        }
        {
            long expected = 0x123456789abcdefL;
            long value = BluetoothMapUtils.getLongFromString(" 1234 5678 9abc-def");
            assertTrue("expected: " + expected + " value = " + value, value == expected);
        }
    }
}
