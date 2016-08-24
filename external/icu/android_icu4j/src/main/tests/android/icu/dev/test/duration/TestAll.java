/* GENERATED SOURCE. DO NOT MODIFY. */
// Copyright 2006 Google Inc.  All Rights Reserved.
/*
******************************************************************************
* Copyright (C) 2007, International Business Machines Corporation and        *
* others. All Rights Reserved.                                               *
******************************************************************************
*/


package android.icu.dev.test.duration;

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
                  "android.icu.dev.test.duration.ICUDurationTest",
                  "android.icu.dev.test.duration.DataReadWriteTest",
                  "android.icu.dev.test.duration.PeriodBuilderFactoryTest",
                  "android.icu.dev.test.duration.PeriodBuilderTest",
                  "android.icu.dev.test.duration.PeriodTest",
                  "android.icu.dev.test.duration.ResourceBasedPeriodFormatterDataServiceTest",
                  "android.icu.dev.test.duration.languages.TestAll",
              },
              "Duration Tests");
    }

    public static final String CLASS_TARGET_NAME = "Duration";
}

