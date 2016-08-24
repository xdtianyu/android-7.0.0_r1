/* GENERATED SOURCE. DO NOT MODIFY. */
/*
 *******************************************************************************
 * Copyright (c) 2004-2016, International Business Machines
 * Corporation and others.  All Rights Reserved.
 * Copyright (C) 2010 , Yahoo! Inc.                                            
 *******************************************************************************
 */
package android.icu.dev.test.format;

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
                  "TestAll$RBNF",
                  "TestAll$NumberFormat",
                  "TestAll$DateFormat",
                  "TestAll$DateIntervalFormat",
                  "TestAll$TimeUnitFormat",
                  "android.icu.dev.test.format.BigNumberFormatTest",
                  "DataDrivenFormatTest",
                  "ListFormatterTest",
                  "MeasureUnitTest",
                  "TestAll$PluralFormat",
                  "TestAll$MessageFormat",
                  "TestAll$SelectFormat"
              },
              "Formatting Tests");
    }

    @RunWith(IcuTestGroupRunner.class)
	public static class RBNF extends TestGroup {
        public RBNF() {
            super(new String[] {
                "RbnfTest",
                "RbnfRoundTripTest",
                "RBNFParseTest",
            });
        }
    }

    @RunWith(IcuTestGroupRunner.class)
	public static class NumberFormat extends TestGroup {
        public NumberFormat() {
            super(new String[] {
                "CompactDecimalFormatTest",
                "IntlTestNumberFormat",
                "IntlTestNumberFormatAPI",
                "NumberFormatSpecificationTest",
                "NumberFormatTest",
                "NumberFormatRegistrationTest",
                "NumberFormatRoundTripTest",
                "NumberRegression",
                "NumberFormatRegressionTest",
                "ScientificNumberFormatterTest",
                "IntlTestDecimalFormatAPI",
                "IntlTestDecimalFormatAPIC",
                "IntlTestDecimalFormatSymbols",
                "IntlTestDecimalFormatSymbolsC",
            });
        }
    }

    @RunWith(IcuTestGroupRunner.class)
	public static class DateFormat extends TestGroup {
        public DateFormat() {
            super(new String[] {
                "DateFormatMiscTests",
                "DateFormatRegressionTest",
                "DateFormatRoundTripTest",
                "DateFormatTest",
                "IntlTestDateFormat",
                "IntlTestDateFormatAPI",
                "IntlTestDateFormatAPIC",
                "IntlTestDateFormatSymbols",
                "DateTimeGeneratorTest",
                "IntlTestSimpleDateFormatAPI",
                "DateFormatRegressionTestJ",
                "TimeZoneFormatTest",
                "RelativeDateTimeFormatterTest"
            });
        }
    }
    
    @RunWith(IcuTestGroupRunner.class)
	public static class DateIntervalFormat extends TestGroup {
        public DateIntervalFormat() {
            super(new String[] {
                "DateIntervalFormatTest"
            });
        }
    }
    
    @RunWith(IcuTestGroupRunner.class)
	public static class TimeUnitFormat extends TestGroup {
        public TimeUnitFormat() {
            super(new String[] {
                "TimeUnitTest"
            });
        }
    }
    
    @RunWith(IcuTestGroupRunner.class)
	public static class PluralFormat extends TestGroup {
        public PluralFormat() {
            super(new String[] {
                "PluralFormatUnitTest",
                "PluralFormatTest",
                "PluralRulesTest",
                "PluralRangesTest",
            });
        }
    }

    @RunWith(IcuTestGroupRunner.class)
	public static class SelectFormat extends TestGroup {
        public SelectFormat() {
            super(new String[] {
                "SelectFormatUnitTest",
                "SelectFormatAPITest",
            });
        }
    }

    @RunWith(IcuTestGroupRunner.class)
	public static class MessageFormat extends TestGroup {
        public MessageFormat() {
            super(new String[] {
                "TestMessageFormat",
                "MessageRegression",
                "MessagePatternUtilTest",
            });
        }
    }

    public static final String CLASS_TARGET_NAME = "Format";
}
