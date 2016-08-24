/* GENERATED SOURCE. DO NOT MODIFY. */
/*
******************************************************************************
* Copyright (C) 2007-2008, International Business Machines Corporation and   *
* others. All Rights Reserved.                                               *
******************************************************************************
*/

// Copyright 2006 Google Inc.  All Rights Reserved.

package android.icu.dev.test.duration.languages;

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
        super(new String[] {
"android.icu.dev.test.duration.languages.Test_en",
                  "android.icu.dev.test.duration.languages.Test_es",
                  "android.icu.dev.test.duration.languages.Test_fr",
                  "android.icu.dev.test.duration.languages.Test_he_IL",
                  "android.icu.dev.test.duration.languages.Test_hi",
                  "android.icu.dev.test.duration.languages.Test_it",
                  "android.icu.dev.test.duration.languages.Test_ja",
                  "android.icu.dev.test.duration.languages.Test_ko",
                  "android.icu.dev.test.duration.languages.Test_zh_Hans",
                  "android.icu.dev.test.duration.languages.Test_zh_Hans_SG",
                  "android.icu.dev.test.duration.languages.Test_zh_Hant",
                  "android.icu.dev.test.duration.languages.Test_zh_Hant_HK",
              },
              "Duration Language Tests");
    }

    public static final String CLASS_TARGET_NAME = "DurationLanguages";
}

