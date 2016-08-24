/* GENERATED SOURCE. DO NOT MODIFY. */
/*
 *******************************************************************************
 * Copyright (C) 2009, International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package android.icu.dev.test;

import android.icu.dev.test.TestFmwk.TestGroup;
import org.junit.runner.RunWith;
import android.icu.junit.IcuTestGroupRunner;

@RunWith(IcuTestGroupRunner.class)
public class TestPackaging extends TestGroup {

    public static void main(String[] args) {
        new TestPackaging().run(args);
    }

    public TestPackaging() {
        super(testList(), "ICU Packaging tests");
    }

    public static String[] testList() {
        return new String[] { "TestLocaleNamePackaging" };
    }

    public static final String CLASS_TARGET_NAME  = "Packaging";
}
