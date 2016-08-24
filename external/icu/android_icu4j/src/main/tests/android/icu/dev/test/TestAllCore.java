/* GENERATED SOURCE. DO NOT MODIFY. */
/*
 *******************************************************************************
 * Copyright (C) 2010, International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package android.icu.dev.test;

import android.icu.dev.test.TestFmwk.TestGroup;
import org.junit.runner.RunWith;
import android.icu.junit.IcuTestGroupRunner;

@RunWith(IcuTestGroupRunner.class)
public class TestAllCore extends TestGroup {

    public static void main(String[] args) {
        new TestAllCore().run(args);
    }

    public TestAllCore() {
        super(
              new String[] {
                  "android.icu.dev.test.format.TestAll",
                  "android.icu.dev.test.compression.TestAll",
                  "android.icu.dev.test.rbbi.TestAll",
                  "android.icu.dev.test.shaping.ArabicShapingRegTest",
                  "android.icu.dev.test.calendar.TestAll",
                  "android.icu.dev.test.timezone.TestAll",
                  "android.icu.dev.test.lang.TestAll",
                  "android.icu.dev.test.text.TestAll",
                  "android.icu.dev.test.normalizer.TestAll",
                  "android.icu.dev.test.util.TestAll",
                  "android.icu.dev.test.iterator.TestUCharacterIterator", // not a group
                  "android.icu.dev.test.bigdec.DiagBigDecimal", // not a group
                  "android.icu.dev.test.impl.TestAll",
                  "android.icu.dev.test.stringprep.TestAll",
                  "android.icu.dev.test.timescale.TestAll",
                  "android.icu.dev.test.charsetdet.TestCharsetDetector",
                  "android.icu.dev.test.bidi.TestAll",
                  "android.icu.dev.test.duration.TestAll",
                  "android.icu.dev.test.serializable.SerializableTest"
              },
              "All core tests in ICU");
    }

    public static final String CLASS_TARGET_NAME  = "Core";
}
