/* GENERATED SOURCE. DO NOT MODIFY. */
/*
 *******************************************************************************
 * Copyright (C) 2003-2010, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
*/
package android.icu.dev.test.stringprep;

import android.icu.dev.test.TestFmwk.TestGroup;
import org.junit.runner.RunWith;
import android.icu.junit.IcuTestGroupRunner;

/**
 * @author ram
 *
 * To change the template for this generated type comment go to
 * Window>Preferences>Java>Code Generation>Code and Comments
 */
@RunWith(IcuTestGroupRunner.class)
public class TestAll extends TestGroup {

    public static void main(String[] args) throws Exception {
        new TestAll().run(args);
    }

    public TestAll() {
        super(
            new String[] {
                "TestIDNA",
                "TestStringPrep",
                "TestIDNARef",
                "IDNAConformanceTest",
                "TestStringPrepProfiles",
            },
            "StringPrep and IDNA test"
            );
    }

    public static final String CLASS_TARGET_NAME = "StringPrep";
}
