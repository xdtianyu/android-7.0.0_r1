/* GENERATED SOURCE. DO NOT MODIFY. */
/*
 **************************************************************************
 * Copyright (C) 2004, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                           *
 **************************************************************************
 *
 */

package android.icu.dev.test.timescale;

import android.icu.dev.test.TestFmwk.TestGroup;
import org.junit.runner.RunWith;
import android.icu.junit.IcuTestGroupRunner;

/**
 * Top level test used to run time scale tests as a batch.
 */
@RunWith(IcuTestGroupRunner.class)
public class TestAll extends TestGroup {

    public TestAll() {
        super(
            new String[] {
                "TimeScaleAPITest",
                "TimeScaleDataTest",
                "TimeScaleMonkeyTest",
            },
            "All TimeScale tests");
    }

    public static void main(String[] args)
    {
        new TestAll().run(args);
    }

    public static final String CLASS_TARGET_NAME  = "TimeScale";
}
