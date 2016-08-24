/* GENERATED SOURCE. DO NOT MODIFY. */
/*
 *******************************************************************************
 * Copyright (C) 1996-2012, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package android.icu.dev.test.calendar;
import android.icu.dev.test.TestFmwk.TestGroup;
import org.junit.runner.RunWith;
import android.icu.junit.IcuTestGroupRunner;

/**
 * Top level test used to run all other calendar tests as a batch.
 */
@RunWith(IcuTestGroupRunner.class)
public class TestAll extends TestGroup {
    public static void main(String[] args) {
        new TestAll().run(args);
    }

    public TestAll() {
        super(
              new String[] {
                  "AstroTest",
                  "CalendarRegression",
                  "CompatibilityTest",
                  "CopticTest",
                  "EthiopicTest",
                  "HebrewTest",
                  "IBMCalendarTest",
                  "IslamicTest",
                  "JapaneseTest",
                  "ChineseTest",
                  "IndianTest",
                  "PersianTest",
                  "HolidayTest",
                  "DataDrivenCalendarTest"
              },
              "Calendars, Holiday, and Astro tests"
              );
    }

    public static final String CLASS_TARGET_NAME = "Calendar";
}
