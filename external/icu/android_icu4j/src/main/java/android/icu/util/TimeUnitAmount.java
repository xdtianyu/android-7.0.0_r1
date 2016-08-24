/* GENERATED SOURCE. DO NOT MODIFY. */
/*
 **************************************************************************
 * Copyright (C) 2008-2009, Google, International Business Machines
 * Corporation and others. All Rights Reserved.
 **************************************************************************
 */
package android.icu.util;

/**
 * Express a duration as a time unit and number. Patterned after Currency.
 * <p>Immutable.
 * @see TimeUnitAmount
 * @see android.icu.text.TimeUnitFormat
 * @author markdavis
 * @hide Only a subset of ICU is exposed in Android
 */
public class TimeUnitAmount extends Measure {

    /**
     * Create from a number and unit.
     */
    public TimeUnitAmount(Number number, TimeUnit unit) {
        super(number, unit);
    }

    /**
     * Create from a number and unit.
     */
    public TimeUnitAmount(double number, TimeUnit unit) {
        super(new Double(number), unit);
    }

    /**
     * Get the unit (convenience to avoid cast).
     */
    public TimeUnit getTimeUnit() {
        return (TimeUnit) getUnit();
    }
}
