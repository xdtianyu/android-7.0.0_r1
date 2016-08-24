/* GENERATED SOURCE. DO NOT MODIFY. */
/*
 *******************************************************************************
 * Copyright (C) 2004-2015, Google Inc, International Business Machines        *
 * Corporation and others. All Rights Reserved.                                *
 *******************************************************************************
 */
package android.icu.util;

import java.io.Externalizable;
import java.io.IOException;
import java.io.ObjectInput;
import java.io.ObjectOutput;
import java.io.ObjectStreamException;
import java.io.Serializable;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.MissingResourceException;
import java.util.Set;

import android.icu.impl.ICUResourceBundle;
import android.icu.impl.Pair;
import android.icu.text.UnicodeSet;

/**
 * A unit such as length, mass, volume, currency, etc.  A unit is
 * coupled with a numeric amount to produce a Measure. MeasureUnit objects are immutable.
 * All subclasses must guarantee that. (However, subclassing is discouraged.)

 *
 * @see android.icu.util.Measure
 * @author Alan Liu
 */
public class MeasureUnit implements Serializable {
    private static final long serialVersionUID = -1839973855554750484L;
    
    // Used to pre-fill the cache. These same constants appear in MeasureFormat too.
    private static final String[] unitKeys = new String[]{"units", "unitsShort", "unitsNarrow"};
    
    private static final Map<String, Map<String,MeasureUnit>> cache 
    = new HashMap<String, Map<String,MeasureUnit>>();

    /**
     * @deprecated This API is ICU internal only.
     * @hide original deprecated declaration
     * @hide draft / provisional / internal are hidden on Android
     */
    @Deprecated
    protected final String type;
    
    /**
     * @deprecated This API is ICU internal only.
     * @hide original deprecated declaration
     * @hide draft / provisional / internal are hidden on Android
     */
    @Deprecated
    protected final String subType;
    
    /**
     * @deprecated This API is ICU internal only.
     * @hide original deprecated declaration
     * @hide draft / provisional / internal are hidden on Android
     */
    @Deprecated
    protected MeasureUnit(String type, String subType) {
        this.type = type;
        this.subType = subType;
    }
    
    /**
     * Get the type, such as "length"
     */
    public String getType() {
        return type;
    }
    

    /**
     * Get the subType, such as “foot”.
     */
    public String getSubtype() {
        return subType;
    }
    
    

    /**
     * {@inheritDoc}
     */
    @Override
    public int hashCode() {
        return 31 * type.hashCode() + subType.hashCode();
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object rhs) {
        if (rhs == this) {
            return true;
        }
        if (!(rhs instanceof MeasureUnit)) {
            return false;
        }
        MeasureUnit c = (MeasureUnit) rhs;
        return type.equals(c.type) && subType.equals(c.subType);
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public String toString() {
        return type + "-" + subType;
    }
    
    /**
     * Get all of the available units' types. Returned set is unmodifiable.
     */
    public synchronized static Set<String> getAvailableTypes() {
        return Collections.unmodifiableSet(cache.keySet());
    }

    /**
     * For the given type, return the available units.
     * @param type the type
     * @return the available units for type. Returned set is unmodifiable.
     */
    public synchronized static Set<MeasureUnit> getAvailable(String type) {
        Map<String, MeasureUnit> units = cache.get(type);
        // Train users not to modify returned set from the start giving us more
        // flexibility for implementation.
        return units == null ? Collections.<MeasureUnit>emptySet()
                : Collections.unmodifiableSet(new HashSet<MeasureUnit>(units.values()));
    }

    /**
     * Get all of the available units. Returned set is unmodifiable.
     */
    public synchronized static Set<MeasureUnit> getAvailable() {
        Set<MeasureUnit> result = new HashSet<MeasureUnit>();
        for (String type : new HashSet<String>(MeasureUnit.getAvailableTypes())) {
            for (MeasureUnit unit : MeasureUnit.getAvailable(type)) {
                result.add(unit);
            }
        }
        // Train users not to modify returned set from the start giving us more
        // flexibility for implementation.
        return Collections.unmodifiableSet(result);
    }

    /**
     * Create a MeasureUnit instance (creates a singleton instance).
     * <p>
     * Normally this method should not be used, since there will be no formatting data
     * available for it, and it may not be returned by getAvailable().
     * However, for special purposes (such as CLDR tooling), it is available.
     *
     * @deprecated This API is ICU internal only.
     * @hide original deprecated declaration
     * @hide draft / provisional / internal are hidden on Android
     */
    @Deprecated
    public static MeasureUnit internalGetInstance(String type, String subType) {
        if (type == null || subType == null) {
            throw new NullPointerException("Type and subType must be non-null");
        }
        if (!"currency".equals(type)) {
            if (!ASCII.containsAll(type) || !ASCII_HYPHEN_DIGITS.containsAll(subType)) {
                throw new IllegalArgumentException("The type or subType are invalid.");
            }
        }
        Factory factory;
        if ("currency".equals(type)) {
            factory = CURRENCY_FACTORY;
        } else if ("duration".equals(type)) {
            factory = TIMEUNIT_FACTORY;
        } else {
            factory = UNIT_FACTORY;
        }
        return MeasureUnit.addUnit(type, subType, factory);
    }
    
    /**
     * For ICU use only.
     * @deprecated This API is ICU internal only.
     * @hide original deprecated declaration
     * @hide draft / provisional / internal are hidden on Android
     */
    @Deprecated
    public static MeasureUnit resolveUnitPerUnit(MeasureUnit unit, MeasureUnit perUnit) {
        return unitPerUnitToSingleUnit.get(Pair.of(unit, perUnit));
    }

    static final UnicodeSet ASCII = new UnicodeSet('a', 'z').freeze();
    static final UnicodeSet ASCII_HYPHEN_DIGITS = new UnicodeSet('-', '-', '0', '9', 'a', 'z').freeze();

    /**
     * @deprecated This API is ICU internal only.
     * @hide original deprecated declaration
     * @hide draft / provisional / internal are hidden on Android
     */
    @Deprecated
    protected interface Factory {
        /**
         * @deprecated This API is ICU internal only.
         * @hide original deprecated declaration
         * @hide draft / provisional / internal are hidden on Android
         */
        @Deprecated
        MeasureUnit create(String type, String subType);
    }

    private static Factory UNIT_FACTORY = new Factory() {
        public MeasureUnit create(String type, String subType) {
            return new MeasureUnit(type, subType);
        }
    };

    static Factory CURRENCY_FACTORY = new Factory() {
        public MeasureUnit create(String unusedType, String subType) {
            return new Currency(subType);
        }
    };
    
    static Factory TIMEUNIT_FACTORY = new Factory() {
        public MeasureUnit create(String type, String subType) {
           return new TimeUnit(type, subType);
        }
    };

    static {
        // load all of the units for English, since we know that that is a superset.
        /**
         *     units{
         *            duration{
         *                day{
         *                    one{"{0} ден"}
         *                    other{"{0} дена"}
         *                }
         */
        ICUResourceBundle resource = (ICUResourceBundle)UResourceBundle.getBundleInstance(ICUResourceBundle.ICU_BASE_NAME, "en");
        for (String key : unitKeys) {
            try {
                ICUResourceBundle unitsTypeRes = resource.getWithFallback(key);
                int size = unitsTypeRes.getSize();
                for ( int index = 0; index < size; ++index) {
                    UResourceBundle unitsRes = unitsTypeRes.get(index);
                    String type = unitsRes.getKey();
                    if (type.equals("compound")) {
                        continue; // special type, does not have any unit plurals
                    }
                    int unitsSize = unitsRes.getSize();
                    for ( int index2 = 0; index2 < unitsSize; ++index2) {
                        ICUResourceBundle unitNameRes = (ICUResourceBundle)unitsRes.get(index2);
                        if (unitNameRes.get("other") != null) {
                            internalGetInstance(type, unitNameRes.getKey());
                        }
                    }
                }
            } catch ( MissingResourceException e ) {
                continue;
            }
        }
        // preallocate currencies
        try {
            UResourceBundle bundle = UResourceBundle.getBundleInstance(
                    ICUResourceBundle.ICU_BASE_NAME,
                    "currencyNumericCodes",
                    ICUResourceBundle.ICU_DATA_CLASS_LOADER);
            UResourceBundle codeMap = bundle.get("codeMap");
            for (Enumeration<String> it = codeMap.getKeys(); it.hasMoreElements();) {
                MeasureUnit.internalGetInstance("currency", it.nextElement());
            }
        } catch (MissingResourceException e) {
            // fall through
        }
    }

    // Must only be called at static initialization, or inside synchronized block.
    /**
     * @deprecated This API is ICU internal only.
     * @hide original deprecated declaration
     * @hide draft / provisional / internal are hidden on Android
     */
    @Deprecated
    protected synchronized static MeasureUnit addUnit(String type, String unitName, Factory factory) {
        Map<String, MeasureUnit> tmp = cache.get(type);
        if (tmp == null) {
            cache.put(type, tmp = new HashMap<String, MeasureUnit>());
        } else {
            // "intern" the type by setting to first item's type.
            type = tmp.entrySet().iterator().next().getValue().type; 
        }
        MeasureUnit unit = tmp.get(unitName);
        if (unit == null) {
            tmp.put(unitName, unit = factory.create(type, unitName));
        }
        return unit;
    }


    /*
     * Useful constants. Not necessarily complete: see {@link #getAvailable()}.
     */
    
// All code between the "Start generated MeasureUnit constants" comment and
// the "End generated MeasureUnit constants" comment is auto generated code
// and must not be edited manually. For instructions on how to correctly
// update this code, refer to:
// http://site.icu-project.org/design/formatting/measureformat/updating-measure-unit
//
    // Start generated MeasureUnit constants
    
    /**
     * Constant for unit of acceleration: g-force
     */
    public static final MeasureUnit G_FORCE = MeasureUnit.internalGetInstance("acceleration", "g-force");

    /**
     * Constant for unit of acceleration: meter-per-second-squared
     */
    public static final MeasureUnit METER_PER_SECOND_SQUARED = MeasureUnit.internalGetInstance("acceleration", "meter-per-second-squared");

    /**
     * Constant for unit of angle: arc-minute
     */
    public static final MeasureUnit ARC_MINUTE = MeasureUnit.internalGetInstance("angle", "arc-minute");

    /**
     * Constant for unit of angle: arc-second
     */
    public static final MeasureUnit ARC_SECOND = MeasureUnit.internalGetInstance("angle", "arc-second");

    /**
     * Constant for unit of angle: degree
     */
    public static final MeasureUnit DEGREE = MeasureUnit.internalGetInstance("angle", "degree");

    /**
     * Constant for unit of angle: radian
     */
    public static final MeasureUnit RADIAN = MeasureUnit.internalGetInstance("angle", "radian");

    /**
     * Constant for unit of angle: revolution
     * @hide draft / provisional / internal are hidden on Android
     */
    public static final MeasureUnit REVOLUTION_ANGLE = MeasureUnit.internalGetInstance("angle", "revolution");

    /**
     * Constant for unit of area: acre
     */
    public static final MeasureUnit ACRE = MeasureUnit.internalGetInstance("area", "acre");

    /**
     * Constant for unit of area: hectare
     */
    public static final MeasureUnit HECTARE = MeasureUnit.internalGetInstance("area", "hectare");

    /**
     * Constant for unit of area: square-centimeter
     */
    public static final MeasureUnit SQUARE_CENTIMETER = MeasureUnit.internalGetInstance("area", "square-centimeter");

    /**
     * Constant for unit of area: square-foot
     */
    public static final MeasureUnit SQUARE_FOOT = MeasureUnit.internalGetInstance("area", "square-foot");

    /**
     * Constant for unit of area: square-inch
     */
    public static final MeasureUnit SQUARE_INCH = MeasureUnit.internalGetInstance("area", "square-inch");

    /**
     * Constant for unit of area: square-kilometer
     */
    public static final MeasureUnit SQUARE_KILOMETER = MeasureUnit.internalGetInstance("area", "square-kilometer");

    /**
     * Constant for unit of area: square-meter
     */
    public static final MeasureUnit SQUARE_METER = MeasureUnit.internalGetInstance("area", "square-meter");

    /**
     * Constant for unit of area: square-mile
     */
    public static final MeasureUnit SQUARE_MILE = MeasureUnit.internalGetInstance("area", "square-mile");

    /**
     * Constant for unit of area: square-yard
     */
    public static final MeasureUnit SQUARE_YARD = MeasureUnit.internalGetInstance("area", "square-yard");

    /**
     * Constant for unit of consumption: liter-per-100kilometers
     * @hide draft / provisional / internal are hidden on Android
     */
    public static final MeasureUnit LITER_PER_100KILOMETERS = MeasureUnit.internalGetInstance("consumption", "liter-per-100kilometers");

    /**
     * Constant for unit of consumption: liter-per-kilometer
     */
    public static final MeasureUnit LITER_PER_KILOMETER = MeasureUnit.internalGetInstance("consumption", "liter-per-kilometer");

    /**
     * Constant for unit of consumption: mile-per-gallon
     */
    public static final MeasureUnit MILE_PER_GALLON = MeasureUnit.internalGetInstance("consumption", "mile-per-gallon");

    /**
     * Constant for unit of digital: bit
     */
    public static final MeasureUnit BIT = MeasureUnit.internalGetInstance("digital", "bit");

    /**
     * Constant for unit of digital: byte
     */
    public static final MeasureUnit BYTE = MeasureUnit.internalGetInstance("digital", "byte");

    /**
     * Constant for unit of digital: gigabit
     */
    public static final MeasureUnit GIGABIT = MeasureUnit.internalGetInstance("digital", "gigabit");

    /**
     * Constant for unit of digital: gigabyte
     */
    public static final MeasureUnit GIGABYTE = MeasureUnit.internalGetInstance("digital", "gigabyte");

    /**
     * Constant for unit of digital: kilobit
     */
    public static final MeasureUnit KILOBIT = MeasureUnit.internalGetInstance("digital", "kilobit");

    /**
     * Constant for unit of digital: kilobyte
     */
    public static final MeasureUnit KILOBYTE = MeasureUnit.internalGetInstance("digital", "kilobyte");

    /**
     * Constant for unit of digital: megabit
     */
    public static final MeasureUnit MEGABIT = MeasureUnit.internalGetInstance("digital", "megabit");

    /**
     * Constant for unit of digital: megabyte
     */
    public static final MeasureUnit MEGABYTE = MeasureUnit.internalGetInstance("digital", "megabyte");

    /**
     * Constant for unit of digital: terabit
     */
    public static final MeasureUnit TERABIT = MeasureUnit.internalGetInstance("digital", "terabit");

    /**
     * Constant for unit of digital: terabyte
     */
    public static final MeasureUnit TERABYTE = MeasureUnit.internalGetInstance("digital", "terabyte");

    /**
     * Constant for unit of duration: century
     * @hide draft / provisional / internal are hidden on Android
     */
    public static final MeasureUnit CENTURY = MeasureUnit.internalGetInstance("duration", "century");

    /**
     * Constant for unit of duration: day
     */
    public static final TimeUnit DAY = (TimeUnit) MeasureUnit.internalGetInstance("duration", "day");

    /**
     * Constant for unit of duration: hour
     */
    public static final TimeUnit HOUR = (TimeUnit) MeasureUnit.internalGetInstance("duration", "hour");

    /**
     * Constant for unit of duration: microsecond
     */
    public static final MeasureUnit MICROSECOND = MeasureUnit.internalGetInstance("duration", "microsecond");

    /**
     * Constant for unit of duration: millisecond
     */
    public static final MeasureUnit MILLISECOND = MeasureUnit.internalGetInstance("duration", "millisecond");

    /**
     * Constant for unit of duration: minute
     */
    public static final TimeUnit MINUTE = (TimeUnit) MeasureUnit.internalGetInstance("duration", "minute");

    /**
     * Constant for unit of duration: month
     */
    public static final TimeUnit MONTH = (TimeUnit) MeasureUnit.internalGetInstance("duration", "month");

    /**
     * Constant for unit of duration: nanosecond
     */
    public static final MeasureUnit NANOSECOND = MeasureUnit.internalGetInstance("duration", "nanosecond");

    /**
     * Constant for unit of duration: second
     */
    public static final TimeUnit SECOND = (TimeUnit) MeasureUnit.internalGetInstance("duration", "second");

    /**
     * Constant for unit of duration: week
     */
    public static final TimeUnit WEEK = (TimeUnit) MeasureUnit.internalGetInstance("duration", "week");

    /**
     * Constant for unit of duration: year
     */
    public static final TimeUnit YEAR = (TimeUnit) MeasureUnit.internalGetInstance("duration", "year");

    /**
     * Constant for unit of electric: ampere
     */
    public static final MeasureUnit AMPERE = MeasureUnit.internalGetInstance("electric", "ampere");

    /**
     * Constant for unit of electric: milliampere
     */
    public static final MeasureUnit MILLIAMPERE = MeasureUnit.internalGetInstance("electric", "milliampere");

    /**
     * Constant for unit of electric: ohm
     */
    public static final MeasureUnit OHM = MeasureUnit.internalGetInstance("electric", "ohm");

    /**
     * Constant for unit of electric: volt
     */
    public static final MeasureUnit VOLT = MeasureUnit.internalGetInstance("electric", "volt");

    /**
     * Constant for unit of energy: calorie
     */
    public static final MeasureUnit CALORIE = MeasureUnit.internalGetInstance("energy", "calorie");

    /**
     * Constant for unit of energy: foodcalorie
     */
    public static final MeasureUnit FOODCALORIE = MeasureUnit.internalGetInstance("energy", "foodcalorie");

    /**
     * Constant for unit of energy: joule
     */
    public static final MeasureUnit JOULE = MeasureUnit.internalGetInstance("energy", "joule");

    /**
     * Constant for unit of energy: kilocalorie
     */
    public static final MeasureUnit KILOCALORIE = MeasureUnit.internalGetInstance("energy", "kilocalorie");

    /**
     * Constant for unit of energy: kilojoule
     */
    public static final MeasureUnit KILOJOULE = MeasureUnit.internalGetInstance("energy", "kilojoule");

    /**
     * Constant for unit of energy: kilowatt-hour
     */
    public static final MeasureUnit KILOWATT_HOUR = MeasureUnit.internalGetInstance("energy", "kilowatt-hour");

    /**
     * Constant for unit of frequency: gigahertz
     */
    public static final MeasureUnit GIGAHERTZ = MeasureUnit.internalGetInstance("frequency", "gigahertz");

    /**
     * Constant for unit of frequency: hertz
     */
    public static final MeasureUnit HERTZ = MeasureUnit.internalGetInstance("frequency", "hertz");

    /**
     * Constant for unit of frequency: kilohertz
     */
    public static final MeasureUnit KILOHERTZ = MeasureUnit.internalGetInstance("frequency", "kilohertz");

    /**
     * Constant for unit of frequency: megahertz
     */
    public static final MeasureUnit MEGAHERTZ = MeasureUnit.internalGetInstance("frequency", "megahertz");

    /**
     * Constant for unit of length: astronomical-unit
     */
    public static final MeasureUnit ASTRONOMICAL_UNIT = MeasureUnit.internalGetInstance("length", "astronomical-unit");

    /**
     * Constant for unit of length: centimeter
     */
    public static final MeasureUnit CENTIMETER = MeasureUnit.internalGetInstance("length", "centimeter");

    /**
     * Constant for unit of length: decimeter
     */
    public static final MeasureUnit DECIMETER = MeasureUnit.internalGetInstance("length", "decimeter");

    /**
     * Constant for unit of length: fathom
     */
    public static final MeasureUnit FATHOM = MeasureUnit.internalGetInstance("length", "fathom");

    /**
     * Constant for unit of length: foot
     */
    public static final MeasureUnit FOOT = MeasureUnit.internalGetInstance("length", "foot");

    /**
     * Constant for unit of length: furlong
     */
    public static final MeasureUnit FURLONG = MeasureUnit.internalGetInstance("length", "furlong");

    /**
     * Constant for unit of length: inch
     */
    public static final MeasureUnit INCH = MeasureUnit.internalGetInstance("length", "inch");

    /**
     * Constant for unit of length: kilometer
     */
    public static final MeasureUnit KILOMETER = MeasureUnit.internalGetInstance("length", "kilometer");

    /**
     * Constant for unit of length: light-year
     */
    public static final MeasureUnit LIGHT_YEAR = MeasureUnit.internalGetInstance("length", "light-year");

    /**
     * Constant for unit of length: meter
     */
    public static final MeasureUnit METER = MeasureUnit.internalGetInstance("length", "meter");

    /**
     * Constant for unit of length: micrometer
     */
    public static final MeasureUnit MICROMETER = MeasureUnit.internalGetInstance("length", "micrometer");

    /**
     * Constant for unit of length: mile
     */
    public static final MeasureUnit MILE = MeasureUnit.internalGetInstance("length", "mile");

    /**
     * Constant for unit of length: mile-scandinavian
     * @hide draft / provisional / internal are hidden on Android
     */
    public static final MeasureUnit MILE_SCANDINAVIAN = MeasureUnit.internalGetInstance("length", "mile-scandinavian");

    /**
     * Constant for unit of length: millimeter
     */
    public static final MeasureUnit MILLIMETER = MeasureUnit.internalGetInstance("length", "millimeter");

    /**
     * Constant for unit of length: nanometer
     */
    public static final MeasureUnit NANOMETER = MeasureUnit.internalGetInstance("length", "nanometer");

    /**
     * Constant for unit of length: nautical-mile
     */
    public static final MeasureUnit NAUTICAL_MILE = MeasureUnit.internalGetInstance("length", "nautical-mile");

    /**
     * Constant for unit of length: parsec
     */
    public static final MeasureUnit PARSEC = MeasureUnit.internalGetInstance("length", "parsec");

    /**
     * Constant for unit of length: picometer
     */
    public static final MeasureUnit PICOMETER = MeasureUnit.internalGetInstance("length", "picometer");

    /**
     * Constant for unit of length: yard
     */
    public static final MeasureUnit YARD = MeasureUnit.internalGetInstance("length", "yard");

    /**
     * Constant for unit of light: lux
     */
    public static final MeasureUnit LUX = MeasureUnit.internalGetInstance("light", "lux");

    /**
     * Constant for unit of mass: carat
     */
    public static final MeasureUnit CARAT = MeasureUnit.internalGetInstance("mass", "carat");

    /**
     * Constant for unit of mass: gram
     */
    public static final MeasureUnit GRAM = MeasureUnit.internalGetInstance("mass", "gram");

    /**
     * Constant for unit of mass: kilogram
     */
    public static final MeasureUnit KILOGRAM = MeasureUnit.internalGetInstance("mass", "kilogram");

    /**
     * Constant for unit of mass: metric-ton
     */
    public static final MeasureUnit METRIC_TON = MeasureUnit.internalGetInstance("mass", "metric-ton");

    /**
     * Constant for unit of mass: microgram
     */
    public static final MeasureUnit MICROGRAM = MeasureUnit.internalGetInstance("mass", "microgram");

    /**
     * Constant for unit of mass: milligram
     */
    public static final MeasureUnit MILLIGRAM = MeasureUnit.internalGetInstance("mass", "milligram");

    /**
     * Constant for unit of mass: ounce
     */
    public static final MeasureUnit OUNCE = MeasureUnit.internalGetInstance("mass", "ounce");

    /**
     * Constant for unit of mass: ounce-troy
     */
    public static final MeasureUnit OUNCE_TROY = MeasureUnit.internalGetInstance("mass", "ounce-troy");

    /**
     * Constant for unit of mass: pound
     */
    public static final MeasureUnit POUND = MeasureUnit.internalGetInstance("mass", "pound");

    /**
     * Constant for unit of mass: stone
     */
    public static final MeasureUnit STONE = MeasureUnit.internalGetInstance("mass", "stone");

    /**
     * Constant for unit of mass: ton
     */
    public static final MeasureUnit TON = MeasureUnit.internalGetInstance("mass", "ton");

    /**
     * Constant for unit of power: gigawatt
     */
    public static final MeasureUnit GIGAWATT = MeasureUnit.internalGetInstance("power", "gigawatt");

    /**
     * Constant for unit of power: horsepower
     */
    public static final MeasureUnit HORSEPOWER = MeasureUnit.internalGetInstance("power", "horsepower");

    /**
     * Constant for unit of power: kilowatt
     */
    public static final MeasureUnit KILOWATT = MeasureUnit.internalGetInstance("power", "kilowatt");

    /**
     * Constant for unit of power: megawatt
     */
    public static final MeasureUnit MEGAWATT = MeasureUnit.internalGetInstance("power", "megawatt");

    /**
     * Constant for unit of power: milliwatt
     */
    public static final MeasureUnit MILLIWATT = MeasureUnit.internalGetInstance("power", "milliwatt");

    /**
     * Constant for unit of power: watt
     */
    public static final MeasureUnit WATT = MeasureUnit.internalGetInstance("power", "watt");

    /**
     * Constant for unit of pressure: hectopascal
     */
    public static final MeasureUnit HECTOPASCAL = MeasureUnit.internalGetInstance("pressure", "hectopascal");

    /**
     * Constant for unit of pressure: inch-hg
     */
    public static final MeasureUnit INCH_HG = MeasureUnit.internalGetInstance("pressure", "inch-hg");

    /**
     * Constant for unit of pressure: millibar
     */
    public static final MeasureUnit MILLIBAR = MeasureUnit.internalGetInstance("pressure", "millibar");

    /**
     * Constant for unit of pressure: millimeter-of-mercury
     */
    public static final MeasureUnit MILLIMETER_OF_MERCURY = MeasureUnit.internalGetInstance("pressure", "millimeter-of-mercury");

    /**
     * Constant for unit of pressure: pound-per-square-inch
     */
    public static final MeasureUnit POUND_PER_SQUARE_INCH = MeasureUnit.internalGetInstance("pressure", "pound-per-square-inch");

    /**
     * Constant for unit of proportion: karat
     */
    public static final MeasureUnit KARAT = MeasureUnit.internalGetInstance("proportion", "karat");

    /**
     * Constant for unit of speed: kilometer-per-hour
     */
    public static final MeasureUnit KILOMETER_PER_HOUR = MeasureUnit.internalGetInstance("speed", "kilometer-per-hour");

    /**
     * Constant for unit of speed: knot
     * @hide draft / provisional / internal are hidden on Android
     */
    public static final MeasureUnit KNOT = MeasureUnit.internalGetInstance("speed", "knot");

    /**
     * Constant for unit of speed: meter-per-second
     */
    public static final MeasureUnit METER_PER_SECOND = MeasureUnit.internalGetInstance("speed", "meter-per-second");

    /**
     * Constant for unit of speed: mile-per-hour
     */
    public static final MeasureUnit MILE_PER_HOUR = MeasureUnit.internalGetInstance("speed", "mile-per-hour");

    /**
     * Constant for unit of temperature: celsius
     */
    public static final MeasureUnit CELSIUS = MeasureUnit.internalGetInstance("temperature", "celsius");

    /**
     * Constant for unit of temperature: fahrenheit
     */
    public static final MeasureUnit FAHRENHEIT = MeasureUnit.internalGetInstance("temperature", "fahrenheit");

    /**
     * Constant for unit of temperature: generic
     * @hide draft / provisional / internal are hidden on Android
     */
    public static final MeasureUnit GENERIC_TEMPERATURE = MeasureUnit.internalGetInstance("temperature", "generic");

    /**
     * Constant for unit of temperature: kelvin
     */
    public static final MeasureUnit KELVIN = MeasureUnit.internalGetInstance("temperature", "kelvin");

    /**
     * Constant for unit of volume: acre-foot
     */
    public static final MeasureUnit ACRE_FOOT = MeasureUnit.internalGetInstance("volume", "acre-foot");

    /**
     * Constant for unit of volume: bushel
     */
    public static final MeasureUnit BUSHEL = MeasureUnit.internalGetInstance("volume", "bushel");

    /**
     * Constant for unit of volume: centiliter
     */
    public static final MeasureUnit CENTILITER = MeasureUnit.internalGetInstance("volume", "centiliter");

    /**
     * Constant for unit of volume: cubic-centimeter
     */
    public static final MeasureUnit CUBIC_CENTIMETER = MeasureUnit.internalGetInstance("volume", "cubic-centimeter");

    /**
     * Constant for unit of volume: cubic-foot
     */
    public static final MeasureUnit CUBIC_FOOT = MeasureUnit.internalGetInstance("volume", "cubic-foot");

    /**
     * Constant for unit of volume: cubic-inch
     */
    public static final MeasureUnit CUBIC_INCH = MeasureUnit.internalGetInstance("volume", "cubic-inch");

    /**
     * Constant for unit of volume: cubic-kilometer
     */
    public static final MeasureUnit CUBIC_KILOMETER = MeasureUnit.internalGetInstance("volume", "cubic-kilometer");

    /**
     * Constant for unit of volume: cubic-meter
     */
    public static final MeasureUnit CUBIC_METER = MeasureUnit.internalGetInstance("volume", "cubic-meter");

    /**
     * Constant for unit of volume: cubic-mile
     */
    public static final MeasureUnit CUBIC_MILE = MeasureUnit.internalGetInstance("volume", "cubic-mile");

    /**
     * Constant for unit of volume: cubic-yard
     */
    public static final MeasureUnit CUBIC_YARD = MeasureUnit.internalGetInstance("volume", "cubic-yard");

    /**
     * Constant for unit of volume: cup
     */
    public static final MeasureUnit CUP = MeasureUnit.internalGetInstance("volume", "cup");

    /**
     * Constant for unit of volume: cup-metric
     * @hide draft / provisional / internal are hidden on Android
     */
    public static final MeasureUnit CUP_METRIC = MeasureUnit.internalGetInstance("volume", "cup-metric");

    /**
     * Constant for unit of volume: deciliter
     */
    public static final MeasureUnit DECILITER = MeasureUnit.internalGetInstance("volume", "deciliter");

    /**
     * Constant for unit of volume: fluid-ounce
     */
    public static final MeasureUnit FLUID_OUNCE = MeasureUnit.internalGetInstance("volume", "fluid-ounce");

    /**
     * Constant for unit of volume: gallon
     */
    public static final MeasureUnit GALLON = MeasureUnit.internalGetInstance("volume", "gallon");

    /**
     * Constant for unit of volume: hectoliter
     */
    public static final MeasureUnit HECTOLITER = MeasureUnit.internalGetInstance("volume", "hectoliter");

    /**
     * Constant for unit of volume: liter
     */
    public static final MeasureUnit LITER = MeasureUnit.internalGetInstance("volume", "liter");

    /**
     * Constant for unit of volume: megaliter
     */
    public static final MeasureUnit MEGALITER = MeasureUnit.internalGetInstance("volume", "megaliter");

    /**
     * Constant for unit of volume: milliliter
     */
    public static final MeasureUnit MILLILITER = MeasureUnit.internalGetInstance("volume", "milliliter");

    /**
     * Constant for unit of volume: pint
     */
    public static final MeasureUnit PINT = MeasureUnit.internalGetInstance("volume", "pint");

    /**
     * Constant for unit of volume: pint-metric
     * @hide draft / provisional / internal are hidden on Android
     */
    public static final MeasureUnit PINT_METRIC = MeasureUnit.internalGetInstance("volume", "pint-metric");

    /**
     * Constant for unit of volume: quart
     */
    public static final MeasureUnit QUART = MeasureUnit.internalGetInstance("volume", "quart");

    /**
     * Constant for unit of volume: tablespoon
     */
    public static final MeasureUnit TABLESPOON = MeasureUnit.internalGetInstance("volume", "tablespoon");

    /**
     * Constant for unit of volume: teaspoon
     */
    public static final MeasureUnit TEASPOON = MeasureUnit.internalGetInstance("volume", "teaspoon");

    private static HashMap<Pair<MeasureUnit, MeasureUnit>, MeasureUnit>unitPerUnitToSingleUnit =
            new HashMap<Pair<MeasureUnit, MeasureUnit>, MeasureUnit>();

    static {
        unitPerUnitToSingleUnit.put(Pair.<MeasureUnit, MeasureUnit>of(MeasureUnit.KILOMETER, MeasureUnit.HOUR), MeasureUnit.KILOMETER_PER_HOUR);
        unitPerUnitToSingleUnit.put(Pair.<MeasureUnit, MeasureUnit>of(MeasureUnit.MILE, MeasureUnit.GALLON), MeasureUnit.MILE_PER_GALLON);
        unitPerUnitToSingleUnit.put(Pair.<MeasureUnit, MeasureUnit>of(MeasureUnit.MILE, MeasureUnit.HOUR), MeasureUnit.MILE_PER_HOUR);
        unitPerUnitToSingleUnit.put(Pair.<MeasureUnit, MeasureUnit>of(MeasureUnit.METER, MeasureUnit.SECOND), MeasureUnit.METER_PER_SECOND);
        unitPerUnitToSingleUnit.put(Pair.<MeasureUnit, MeasureUnit>of(MeasureUnit.LITER, MeasureUnit.KILOMETER), MeasureUnit.LITER_PER_KILOMETER);
        unitPerUnitToSingleUnit.put(Pair.<MeasureUnit, MeasureUnit>of(MeasureUnit.POUND, MeasureUnit.SQUARE_INCH), MeasureUnit.POUND_PER_SQUARE_INCH);
    }

    // End generated MeasureUnit constants
    /* Private */

    private Object writeReplace() throws ObjectStreamException {
        return new MeasureUnitProxy(type, subType);
    }

    static final class MeasureUnitProxy implements Externalizable {
        private static final long serialVersionUID = -3910681415330989598L;

        private String type;
        private String subType;

        public MeasureUnitProxy(String type, String subType) {
            this.type = type;
            this.subType = subType;
        }

        // Must have public constructor, to enable Externalizable
        public MeasureUnitProxy() {
        }

        public void writeExternal(ObjectOutput out) throws IOException {
            out.writeByte(0); // version
            out.writeUTF(type);
            out.writeUTF(subType);
            out.writeShort(0); // allow for more data.
        }

        public void readExternal(ObjectInput in) throws IOException, ClassNotFoundException {
            /* byte version = */ in.readByte(); // version
            type = in.readUTF();
            subType = in.readUTF();
            // allow for more data from future version
            int extra = in.readShort();
            if (extra > 0) {
                byte[] extraBytes = new byte[extra];
                in.read(extraBytes, 0, extra);
            }
        }

        private Object readResolve() throws ObjectStreamException {
            return MeasureUnit.internalGetInstance(type, subType);
        }
    }
}
