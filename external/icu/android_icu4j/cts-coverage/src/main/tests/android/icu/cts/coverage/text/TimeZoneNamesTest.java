/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package android.icu.cts.coverage.text;

import android.icu.text.TimeZoneNames;
import android.icu.util.TimeZone;
import android.icu.util.ULocale;
import java.util.Collections;
import java.util.Locale;
import java.util.Set;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

/**
 * Extra tests to improve CTS Test Coverage.
 *
 * Where necessary (i.e. when a method is abstract) it tests the implementations of TimeZoneNames;
 * excluding the default one which shouldn't be used on Android.
 */
@RunWith(JUnit4.class)
public class TimeZoneNamesTest {

    @Test
    public void testGetInstance_Locale() {
        TimeZoneNames uLocaleInstance = TimeZoneNames.getInstance(ULocale.CANADA);
        TimeZoneNames localeInstance = TimeZoneNames.getInstance(Locale.CANADA);

        Set<String> uLocaleAvailableIds = uLocaleInstance.getAvailableMetaZoneIDs();
        Set<String> localeAvailableIds = localeInstance.getAvailableMetaZoneIDs();
        assertEquals("Available ids", uLocaleAvailableIds, localeAvailableIds);

        for (String availableId : uLocaleAvailableIds) {
            long date = 1458385200000L;
            TimeZoneNames.NameType nameType = TimeZoneNames.NameType.SHORT_GENERIC;
            String uLocaleName = uLocaleInstance.getDisplayName(availableId, nameType, date);
            String localeName = localeInstance.getDisplayName(availableId, nameType, date);
            assertEquals("Id: " + availableId, uLocaleName, localeName);
        }
    }

    @Test
    public void testGetAvailableMetaZoneIDs() {
        TimeZoneNames japaneseNames = TimeZoneNames.getInstance(ULocale.JAPANESE);
        Set<String> allJapan = japaneseNames.getAvailableMetaZoneIDs();

        TimeZoneNames tzdbNames = TimeZoneNames.getTZDBInstance(ULocale.CHINESE);
        Set<String> tzdbAll = tzdbNames.getAvailableMetaZoneIDs();

        // The data is the same in the current implementation.
        assertEquals(allJapan, tzdbAll);

        // Make sure that there is something.
        assertTrue("count of zone ids is less than 100", allJapan.size() >= 180);
    }

    @Test
    public void testGetAvailableMetaZoneIDs_String() {
        TimeZoneNames japaneseNames = TimeZoneNames.getInstance(ULocale.JAPANESE);
        assertEquals(Collections.singleton("America_Pacific"),
                japaneseNames.getAvailableMetaZoneIDs("America/Los_Angeles"));

        TimeZoneNames tzdbNames = TimeZoneNames.getTZDBInstance(ULocale.CHINESE);
        assertEquals(Collections.singleton("Taipei"),
                tzdbNames.getAvailableMetaZoneIDs("Asia/Taipei"));
    }

    @Test
    public void testGetMetaZoneDisplayName() {
        TimeZoneNames usNames = TimeZoneNames.getInstance(ULocale.US);

        String europeanCentralName = usNames.getMetaZoneDisplayName("Europe_Central",
                TimeZoneNames.NameType.LONG_STANDARD);
        assertEquals("Central European Standard Time", europeanCentralName);

        TimeZoneNames tzdbNames = TimeZoneNames.getTZDBInstance(ULocale.CHINESE);
        String americaPacificName = tzdbNames.getMetaZoneDisplayName("America_Pacific",
                TimeZoneNames.NameType.SHORT_DAYLIGHT);
        assertEquals("PDT", americaPacificName);
    }

    @Test
    public void testGetMetaZoneID() {
        TimeZoneNames usNames = TimeZoneNames.getInstance(ULocale.US);

        String europeanCentralName = usNames.getMetaZoneID("Europe/Paris", 0);
        assertEquals("Europe_Central", europeanCentralName);

        TimeZoneNames tzdbNames = TimeZoneNames.getTZDBInstance(ULocale.KOREAN);
        String seoulName = tzdbNames.getMetaZoneID("Asia/Seoul", 0);
        assertEquals("Korea", seoulName);

        // Now try Jan 1st 1945 GMT
        seoulName = tzdbNames.getMetaZoneID("Asia/Seoul", -786240000000L);
        assertNull(seoulName);
    }

    @Test
    public void testGetTimeZoneDisplayName() {
        TimeZoneNames frenchNames = TimeZoneNames.getInstance(ULocale.FRENCH);
        String dublinName = frenchNames.getTimeZoneDisplayName("Europe/Dublin",
                TimeZoneNames.NameType.LONG_DAYLIGHT);
        assertEquals("heure d’été irlandaise", dublinName);

        String dublinLocation = frenchNames.getTimeZoneDisplayName("Europe/Dublin",
                TimeZoneNames.NameType.EXEMPLAR_LOCATION);
        assertEquals("Dublin", dublinLocation);

        // All the names returned by this are null.
        TimeZoneNames tzdbNames = TimeZoneNames.getTZDBInstance(ULocale.KOREAN);
        for (String tzId : TimeZone.getAvailableIDs()) {
            for (TimeZoneNames.NameType nameType : TimeZoneNames.NameType.values()) {
                String name = tzdbNames.getTimeZoneDisplayName(tzId, nameType);
                assertNull("TZ:" + tzId + ", NameType: " + nameType + ", value: " + name, name);
            }
        }
    }
}
