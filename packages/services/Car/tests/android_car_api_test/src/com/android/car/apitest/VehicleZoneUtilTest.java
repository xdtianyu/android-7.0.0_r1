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
package com.android.car.apitest;

import android.car.VehicleZoneUtil;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

@SmallTest
public class VehicleZoneUtilTest extends AndroidTestCase {

    public void testZoneToIndex() {
        int zones = 0;
        int zone = 0;
        try {
            int r = VehicleZoneUtil.zoneToIndex(zones, zone);
            fail();
        } catch (IllegalArgumentException e) {
            // expected
        }
        zone = 0x1;
        try {
            int r = VehicleZoneUtil.zoneToIndex(zones, zone);
            fail();
        } catch (IllegalArgumentException e) {
            // expected
        }
        zones = 0xffffffff;
        zone = 0x1;
        assertEquals(0, VehicleZoneUtil.zoneToIndex(zones, zone));
        zone = 0x80000000;
        assertEquals(31, VehicleZoneUtil.zoneToIndex(zones, zone));
        zones = 0x1002;
        try {
            int r = VehicleZoneUtil.zoneToIndex(zones, zone);
            fail();
        } catch (IllegalArgumentException e) {
            // expected
        }
        zone = 0x2;
        assertEquals(0, VehicleZoneUtil.zoneToIndex(zones, zone));
        zone = 0x1000;
        assertEquals(1, VehicleZoneUtil.zoneToIndex(zones, zone));
    }

    public void testGetNumBerOfZones() {
        int zones = 0;
        assertEquals(0, VehicleZoneUtil.getNumberOfZones(zones));
        zones = 0x1;
        assertEquals(1, VehicleZoneUtil.getNumberOfZones(zones));
        zones = 0x7;
        assertEquals(3, VehicleZoneUtil.getNumberOfZones(zones));
        zones = 0xffffffff;
        assertEquals(32, VehicleZoneUtil.getNumberOfZones(zones));
    }

    public void testGetFirstZone() {
        int zones = 0;
        assertEquals(0, VehicleZoneUtil.getFirstZone(zones));
        zones = 0x1;
        assertEquals(0x1, VehicleZoneUtil.getFirstZone(zones));
        zones = 0xffff00;
        assertEquals(0x100, VehicleZoneUtil.getFirstZone(zones));
    }

    public void testGetNextZone() {
        int zones = 0;
        int startingZone = 0x1;
        assertEquals(0, VehicleZoneUtil.getNextZone(zones, startingZone));
        startingZone = 0;
        try {
            int r = VehicleZoneUtil.getNextZone(zones, startingZone);
            fail();
        } catch (IllegalArgumentException e) {
            // expected
        }
        zones = 0x1;
        startingZone = 0;
        try {
            int r = VehicleZoneUtil.getNextZone(zones, startingZone);
            fail();
        } catch (IllegalArgumentException e) {
            // expected
        }
        startingZone = 0x1;
        assertEquals(0, VehicleZoneUtil.getNextZone(zones, startingZone));
        zones = 0xff00;
        startingZone = 0x1;
        assertEquals(0x100, VehicleZoneUtil.getNextZone(zones, startingZone));
        startingZone = 0x0100;
        assertEquals(0x0200, VehicleZoneUtil.getNextZone(zones, startingZone));
        zones = 0xf;
        startingZone = 0x2;
        assertEquals(0x4, VehicleZoneUtil.getNextZone(zones, startingZone));
        zones = 0xf0000000;
        startingZone = 0x40000000;
        assertEquals(0x80000000, VehicleZoneUtil.getNextZone(zones, startingZone));
    }

    public void testGetAllZones() {
        int zones = 0;
        int[] list = VehicleZoneUtil.listAllZones(zones);
        assertEquals(0, list.length);
        zones = 0xffffffff;
        list = VehicleZoneUtil.listAllZones(zones);
        assertEquals(32, list.length);
        for (int i = 0; i < 32; i++) {
            assertEquals(0x1<<i, list[i]);
        }
        zones = 0x1001;
        list = VehicleZoneUtil.listAllZones(zones);
        assertEquals(2, list.length);
        assertEquals(0x1, list[0]);
        assertEquals(0x1000, list[1]);
    }
}
