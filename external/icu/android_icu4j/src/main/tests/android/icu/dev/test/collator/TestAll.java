/* GENERATED SOURCE. DO NOT MODIFY. */
/*
 *******************************************************************************
 * Copyright (C) 1996-2010, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package android.icu.dev.test.collator;

import android.icu.dev.test.TestFmwk.TestGroup;
import org.junit.runner.RunWith;
import android.icu.junit.IcuTestGroupRunner;

/**
 * Top level test used to run all collation and search tests as a batch.
 */
@RunWith(IcuTestGroupRunner.class)
public class TestAll extends TestGroup {
    public static void main(String[] args) {
        new TestAll().run(args);
    }

    public TestAll() {
        super(
              new String[] {
                  "CollationTest",
                  "CollationAPITest",
                  "CollationCurrencyTest",
                  "CollationCreationMethodTest",
                  //"CollationDanishTest", //Danish is already tested through data driven tests
                  "CollationDummyTest",
                  "CollationEnglishTest",
                  "CollationFinnishTest",
                  "CollationFrenchTest",
                  "CollationGermanTest",
                  "CollationIteratorTest",
                  "CollationKanaTest",
                  "CollationMonkeyTest",
                  "CollationRegressionTest",
                  "CollationSpanishTest",
                  "CollationThaiTest",
                  "CollationTurkishTest",
                  "G7CollationTest",
                  "LotusCollationKoreanTest",
                  "CollationMiscTest",
                  "CollationChineseTest",
                  "CollationServiceTest",
                  "CollationThreadTest",
                  //"RandomCollator", //Disabled until the problem in the test case is resolved #5747
                  "UCAConformanceTest",
                  // don't test Search API twice!
                  //"android.icu.dev.test.search.SearchTest"
                  "AlphabeticIndexTest"
              },
              "All Collation Tests"
              );
    }

    public static final String CLASS_TARGET_NAME = "Collator";
}
