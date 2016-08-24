/* GENERATED SOURCE. DO NOT MODIFY. */
/*
*******************************************************************************
*   Copyright (C) 2001-2012, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*/

package android.icu.dev.test.bidi;

import android.icu.dev.test.TestFmwk.TestGroup;
import org.junit.runner.RunWith;
import android.icu.junit.IcuTestGroupRunner;

/**
 * Top level test used to run all other tests as a batch.
 */
@RunWith(IcuTestGroupRunner.class)
public class TestAll extends TestGroup {

    public static void main(String[] args) {
        new TestAll().run(args);
    }

    public TestAll() {
        super(
              new String[] {
                  "android.icu.dev.test.bidi.TestCharFromDirProp",
                  "android.icu.dev.test.bidi.TestBidi",
                  "android.icu.dev.test.bidi.TestInverse",
                  "android.icu.dev.test.bidi.TestReorder",
                  "android.icu.dev.test.bidi.TestReorderArabicMathSymbols",
                  "android.icu.dev.test.bidi.TestFailureRecovery",
                  "android.icu.dev.test.bidi.TestMultipleParagraphs",
                  "android.icu.dev.test.bidi.TestReorderingMode",
                  "android.icu.dev.test.bidi.TestReorderRunsOnly",
                  "android.icu.dev.test.bidi.TestStreaming",
                  "android.icu.dev.test.bidi.TestClassOverride",
                  "android.icu.dev.test.bidi.TestCompatibility",
                  "android.icu.dev.test.bidi.TestContext",
                  "android.icu.dev.test.bidi.BiDiConformanceTest"
              },
              "Bidi tests");
    }

    public static final String CLASS_TARGET_NAME  = "Bidi";
}
