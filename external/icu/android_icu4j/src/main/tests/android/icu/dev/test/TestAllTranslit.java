/* GENERATED SOURCE. DO NOT MODIFY. */
/*
 *******************************************************************************
 * Copyright (C) 1996-2009, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package android.icu.dev.test;

import android.icu.dev.test.TestFmwk.TestGroup;
import org.junit.runner.RunWith;
import android.icu.junit.IcuTestGroupRunner;

/**
 * Top level test used to run all other tests as a batch.
 */
@RunWith(IcuTestGroupRunner.class)
public class TestAllTranslit extends TestGroup {

    public static void main(String[] args) {
        new TestAllTranslit().run(args);
    }

    public TestAllTranslit() {
        super(
              new String[] {
                  "android.icu.dev.test.translit.TestAll",
                  // funky tests of test code
                  // "android.icu.dev.test.util.TestBNF",
                  // "android.icu.dev.test.util.TestBagFormatter",
              },
              "All tests in ICU translit");
    }

    public static final String CLASS_TARGET_NAME  = "Translit";
}
