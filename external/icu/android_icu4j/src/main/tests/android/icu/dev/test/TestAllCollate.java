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
public class TestAllCollate extends TestGroup {

    public static void main(String[] args) {
        new TestAllCollate().run(args);
    }

    public TestAllCollate() {
        super(
              new String[] {
                  "android.icu.dev.test.collator.TestAll",
                  "android.icu.dev.test.format.GlobalizationPreferencesTest",
                  "android.icu.dev.test.format.RbnfLenientScannerTest",
                  "android.icu.dev.test.search.SearchTest",
                  "android.icu.dev.test.util.ICUResourceBundleCollationTest",
                  "android.icu.dev.test.util.LocaleAliasCollationTest",
                  "android.icu.dev.test.util.ULocaleCollationTest",
              },
              "All tests in ICU collation");
    }

    public static final String CLASS_TARGET_NAME  = "Collate";
}
