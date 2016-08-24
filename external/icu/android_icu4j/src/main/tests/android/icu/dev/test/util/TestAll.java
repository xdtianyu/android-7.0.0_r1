/* GENERATED SOURCE. DO NOT MODIFY. */
/*
 *******************************************************************************
 * Copyright (C) 1996-2012, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package android.icu.dev.test.util;

import android.icu.dev.test.TestFmwk.TestGroup;
import org.junit.runner.RunWith;
import android.icu.junit.IcuTestGroupRunner;

/**
 * Top level test used to run all other tests as a batch.
 */
@RunWith(IcuTestGroupRunner.class)
public class TestAll extends TestGroup {
    public static void main(String[] args) throws Exception {
        new TestAll().run(args);
    }

    public TestAll() {
        super(
              new String[] {
            "ICUServiceTest",
            "ICUServiceThreadTest",
            "ICUBinaryTest",
            "SimpleFormatterTest",
            "TextTrieMapTest",
            "VersionInfoTest",
            "ICUResourceBundleTest",
            "BytesTrieTest",
            "CharsTrieTest",
            "CompactArrayTest",
            "StringTokenizerTest",
            "CurrencyTest",
            "UtilityTest",
            "TrieTest",
            "Trie2Test",
            "LocaleDataTest",
            "GenderInfoTest",
            "ULocaleTest",
            "LocaleAliasTest",
            "DebugUtilitiesTest",
            "LocaleBuilderTest",
            "LocaleMatcherTest",
            "LocalePriorityListTest",
            "RegionTest"
        },
              "Test miscellaneous public utilities");
    }

    public static final String CLASS_TARGET_NAME = "Util";
}
