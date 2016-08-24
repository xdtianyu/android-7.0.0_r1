/* GENERATED SOURCE. DO NOT MODIFY. */
/*
 *******************************************************************************
 * Copyright (C) 1996-2013, International Business Machines Corporation and    *
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
public class TestAll extends TestGroup {

    public static void main(String[] args) {
        new TestAll().run(args);
    }

    public TestAll() {
        super(
              new String[] {
                  "android.icu.dev.test.TestAllCore",
                  "android.icu.dev.test.TestAllCollate",
                  "android.icu.dev.test.TestAllTranslit",
                  "android.icu.dev.test.charset.TestAll",
              },
              "All tests in ICU");
    }

    public static final String CLASS_TARGET_NAME  = "ICU";
}
