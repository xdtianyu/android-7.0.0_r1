/* GENERATED SOURCE. DO NOT MODIFY. */
/*
*******************************************************************************
*   Copyright (C) 2007-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*/

package android.icu.dev.test.bidi;

import android.icu.impl.Utility;
import android.icu.lang.UCharacter;
import android.icu.lang.UCharacterDirection;
import org.junit.runner.RunWith;
import android.icu.junit.IcuTestFmwkRunner;

/**
 * Regression test for Bidi charFromDirProp
 *
 * @author Lina Kemmel, Matitiahu Allouche
 */

@RunWith(IcuTestFmwkRunner.class)
public class TestCharFromDirProp extends BidiTest {

    /* verify that the exemplar characters have the expected bidi classes */
    public void testCharFromDirProp() {

        logln("\nEntering TestCharFromDirProp");
        int i = UCharacterDirection.CHAR_DIRECTION_COUNT;
        while (i-- > 0) {
            char c = charFromDirProp[i];
            int dir = UCharacter.getDirection(c);
            assertEquals("UCharacter.getDirection(TestData.charFromDirProp[" + i
                    + "] == U+" + Utility.hex(c) + ") failed", i, dir);
        }
        logln("\nExiting TestCharFromDirProp");
    }

    public static void main(String[] args) {
        try {
            new TestCharFromDirProp().run(args);
        }
        catch (Exception e) {
            System.out.println(e);
        }
    }
}
