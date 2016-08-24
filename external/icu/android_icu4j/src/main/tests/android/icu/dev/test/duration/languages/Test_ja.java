/* GENERATED SOURCE. DO NOT MODIFY. */
/*
******************************************************************************
* Copyright (C) 2007-2010, International Business Machines Corporation and   *
* others. All Rights Reserved.                                               *
******************************************************************************
*/

// Copyright 2006 Google Inc.  All Rights Reserved.

package android.icu.dev.test.duration.languages;

import android.icu.dev.test.duration.LanguageTestRoot;
import android.icu.impl.duration.BasicPeriodFormatterFactory;
import android.icu.impl.duration.Period;
import android.icu.impl.duration.PeriodFormatter;
import android.icu.impl.duration.TimeUnitConstants;
import org.junit.runner.RunWith;
import android.icu.junit.IcuTestFmwkRunner;

/**
 * Test cases for en
 */
@RunWith(IcuTestFmwkRunner.class)
public class Test_ja extends LanguageTestRoot implements TimeUnitConstants {
  private BasicPeriodFormatterFactory pff;

  /**
   * Invoke the tests.
   */
  public static void main(String[] args) {
      new Test_ja().run(args);
  }

  /**
   * Constructor.
   */
  public Test_ja() {
    super("ja", false);
    this.pff = (BasicPeriodFormatterFactory)BasicPeriodFormatterFactory.getDefault().
        setLocale("ja");
  }

  public void testNoMillis() {
    // explicit test that japanese doesn't use both seconds
    // and milliseconds when formatting.  if both appear,
    // force decimal3 seconds and merge millis with it.
      logln("test disabled");
      /*
    PeriodFormatter pf = pff.getFormatter();
    Period p = Period.at(1, MILLISECOND);
    assertEquals(null, "1\u30df\u30ea\u79d2\u524d", pf.format(p));

    p.and(1, SECOND);
    assertEquals(null, "1.001\u79d2\u524d", pf.format(p));

    p.and(1, MINUTE).omit(SECOND);
    assertEquals(null, "1\u5206\u30681\u30df\u30ea\u79d2\u524d", pf.format(p));
      */
  }

  public void testOmitZeros() {
    // zeros are treated as omitted

    PeriodFormatter pf = pff.getFormatter();
    Period p = Period.at(1, MINUTE).and(0, SECOND).and(1, MILLISECOND);
    String s1 = pf.format(p);
    p.omit(SECOND);
    String s2 = pf.format(p);
    assertEquals(null, s1, s2);
  }
}
