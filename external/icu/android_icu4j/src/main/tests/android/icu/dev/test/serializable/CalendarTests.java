/* GENERATED SOURCE. DO NOT MODIFY. */
/*
 *******************************************************************************
 * Copyright (C) 1996-2012, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 *
 */

package android.icu.dev.test.serializable;

import java.util.Locale;

import android.icu.util.BuddhistCalendar;
import android.icu.util.Calendar;
import android.icu.util.ChineseCalendar;
import android.icu.util.CopticCalendar;
import android.icu.util.DangiCalendar;
import android.icu.util.EthiopicCalendar;
import android.icu.util.GregorianCalendar;
import android.icu.util.HebrewCalendar;
import android.icu.util.IndianCalendar;
import android.icu.util.IslamicCalendar;
import android.icu.util.JapaneseCalendar;
import android.icu.util.PersianCalendar;
import android.icu.util.TaiwanCalendar;
import android.icu.util.TimeZone;
import android.icu.util.ULocale;

/**
 * @author emader
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class CalendarTests
{
    static class CalendarHandler implements SerializableTest.Handler
    {
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTest.getLocales();
            TimeZone pst = TimeZone.getTimeZone("America/Los_Angeles");
            Calendar calendars[] = new Calendar[locales.length];
            
            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = Calendar.getInstance(pst, locales[i]);
            }
            
            return calendars;
        }
        
        public boolean hasSameBehavior(Object a, Object b)
        {
            Calendar cal_a = (Calendar) a;
            Calendar cal_b = (Calendar) b;
            long now = System.currentTimeMillis();
            
            cal_a.setTimeInMillis(now);
            cal_a.roll(Calendar.MONTH, 1);
            
            cal_b.setTimeInMillis(now);
            cal_b.roll(Calendar.MONTH, 1);
            
            return cal_a.getTime().equals(cal_a.getTime());
        }
    }

    static class BuddhistCalendarHandler extends CalendarHandler
    {
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTest.getLocales();
            TimeZone tst = TimeZone.getTimeZone("Asia/Bangkok");
            BuddhistCalendar calendars[] = new BuddhistCalendar[locales.length];
            
            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new BuddhistCalendar(tst, locales[i]);
            }
            
            return calendars;
        }
    }
    
    static class ChineseCalendarHandler extends CalendarHandler
    {
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTest.getLocales();
            TimeZone cst = TimeZone.getTimeZone("Asia/Shanghai");
            ChineseCalendar calendars[] = new ChineseCalendar[locales.length];
            
            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new ChineseCalendar(cst, locales[i]);
            }
            
            return calendars; 
        }
    }
    
    static class CopticCalendarHandler extends CalendarHandler
    {
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTest.getLocales();
            TimeZone ast = TimeZone.getTimeZone("Europe/Athens");
            CopticCalendar calendars[] = new CopticCalendar[locales.length];
            
            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new CopticCalendar(ast, locales[i]);
            }
            
            return calendars; 
        }
    }

    static class DangiCalendarHandler extends CalendarHandler
    {
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTest.getLocales();
            TimeZone kst = TimeZone.getTimeZone("Asia/Seoul");
            DangiCalendar calendars[] = new DangiCalendar[locales.length];
            
            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new DangiCalendar(kst, ULocale.forLocale(locales[i]));
            }
            
            return calendars; 
        }
    }

    static class EthiopicCalendarHandler extends CalendarHandler
    {
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTest.getLocales();
            TimeZone ast = TimeZone.getTimeZone("Africa/Addis_Ababa");
            EthiopicCalendar calendars[] = new EthiopicCalendar[locales.length];
            
            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new EthiopicCalendar(ast, locales[i]);
            }
            
            return calendars; 
        }
    }

    static class GregorianCalendarHandler extends CalendarHandler
    {
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTest.getLocales();
            TimeZone pst = TimeZone.getTimeZone("America/Los_Angeles");
            GregorianCalendar calendars[] = new GregorianCalendar[locales.length];
            
            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new GregorianCalendar(pst, locales[i]);
            }
            
            return calendars; 
        }
    }

    static class HebrewCalendarHandler extends CalendarHandler
    {
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTest.getLocales();
            TimeZone jst = TimeZone.getTimeZone("Asia/Jerusalem");
            HebrewCalendar calendars[] = new HebrewCalendar[locales.length];
            
            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new HebrewCalendar(jst, locales[i]);
            }
            
            return calendars; 
        }
    }
    
    static class IndianCalendarHandler extends CalendarHandler
    {
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTest.getLocales();
            TimeZone jst = TimeZone.getTimeZone("Asia/Calcutta");
            IndianCalendar calendars[] = new IndianCalendar[locales.length];
            
            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new IndianCalendar(jst, locales[i]);
            }
            
            return calendars; 
        }
    }
    
    static class IslamicCalendarHandler extends CalendarHandler
    {
        public Object[] getTestObjects() {
            Locale locales[] = SerializableTest.getLocales();
            TimeZone cst = TimeZone.getTimeZone("Africa/Cairo");
            IslamicCalendar calendars[] = new IslamicCalendar[locales.length];
            
            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new IslamicCalendar(cst, locales[i]);
            }
            
            return calendars; 
        }
    }

    static class JapaneseCalendarHandler extends CalendarHandler
    {
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTest.getLocales();
            TimeZone jst = TimeZone.getTimeZone("Asia/Tokyo");
            JapaneseCalendar calendars[] = new JapaneseCalendar[locales.length];
            
            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new JapaneseCalendar(jst, locales[i]);
            }
            
            return calendars; 
        }
    }

    static class PersianCalendarHandler extends CalendarHandler
    {
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTest.getLocales();
            TimeZone kst = TimeZone.getTimeZone("Asia/Tehran");
            PersianCalendar calendars[] = new PersianCalendar[locales.length];
            
            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new PersianCalendar(kst, ULocale.forLocale(locales[i]));
            }
            
            return calendars; 
        }
    }

    static class TaiwanCalendarHandler extends CalendarHandler {
        public Object[] getTestObjects() {
            Locale locales[] = SerializableTest.getLocales();
            TimeZone cst = TimeZone.getTimeZone("Asia/Shanghai");
            TaiwanCalendar calendars[] = new TaiwanCalendar[locales.length];
            
            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new TaiwanCalendar(cst, locales[i]);
            }
            
            return calendars; 
        }
    }

    public static void main(String[] args)
    {
        //nothing needed yet...
    }
}
