/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.internal.telephony.dataconnection;

import android.os.PersistableBundle;
import android.telephony.CarrierConfigManager;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.TelephonyTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;
import static org.junit.Assert.assertEquals;

public class ApnSettingTest extends TelephonyTest {

    private PersistableBundle mBundle;
    private boolean isRoaming = false;

    @Before
    public void setUp() throws Exception {
        super.setUp(getClass().getSimpleName());
        mBundle = mContextFixture.getCarrierConfigBundle();
    }

    @After
    public void tearDown() throws Exception {
        super.tearDown();
    }

    private ApnSetting createApnSetting(String[] apnTypes) {
        return new ApnSetting(
                2163,                   // id
                "44010",                // numeric
                "sp-mode",              // name
                "spmode.ne.jp",         // apn
                "",                     // proxy
                "",                     // port
                "",                     // mmsc
                "",                     // mmsproxy
                "",                     // mmsport
                "",                     // user
                "",                     // password
                -1,                     // authtype
                apnTypes,               // types
                "IP",                   // protocol
                "IP",                   // roaming_protocol
                true,                   // carrier_enabled
                0,                      // bearer
                0,                      // bearer_bitmask
                0,                      // profile_id
                false,                  // modem_cognitive
                0,                      // max_conns
                0,                      // wait_time
                0,                      // max_conns_time
                0,                      // mtu
                "",                     // mvno_type
                "");                    // mnvo_match_data
    }

    private static void assertApnSettingsEqual(List<ApnSetting> a1, List<ApnSetting> a2) {
        assertEquals(a1.size(), a2.size());
        for (int i = 0; i < a1.size(); ++i) {
            assertApnSettingEqual(a1.get(i), a2.get(i));
        }
    }

    private static void assertApnSettingEqual(ApnSetting a1, ApnSetting a2) {
        assertEquals(a1.carrier, a2.carrier);
        assertEquals(a1.apn, a2.apn);
        assertEquals(a1.proxy, a2.proxy);
        assertEquals(a1.port, a2.port);
        assertEquals(a1.mmsc, a2.mmsc);
        assertEquals(a1.mmsProxy, a2.mmsProxy);
        assertEquals(a1.mmsPort, a2.mmsPort);
        assertEquals(a1.user, a2.user);
        assertEquals(a1.password, a2.password);
        assertEquals(a1.authType, a2.authType);
        assertEquals(a1.id, a2.id);
        assertEquals(a1.numeric, a2.numeric);
        assertEquals(a1.protocol, a2.protocol);
        assertEquals(a1.roamingProtocol, a2.roamingProtocol);
        assertEquals(a1.types.length, a2.types.length);
        int i;
        for (i = 0; i < a1.types.length; i++) {
            assertEquals(a1.types[i], a2.types[i]);
        }
        assertEquals(a1.carrierEnabled, a2.carrierEnabled);
        assertEquals(a1.bearerBitmask, a2.bearerBitmask);
        assertEquals(a1.profileId, a2.profileId);
        assertEquals(a1.modemCognitive, a2.modemCognitive);
        assertEquals(a1.maxConns, a2.maxConns);
        assertEquals(a1.waitTime, a2.waitTime);
        assertEquals(a1.maxConnsTime, a2.maxConnsTime);
        assertEquals(a1.mtu, a2.mtu);
        assertEquals(a1.mvnoType, a2.mvnoType);
        assertEquals(a1.mvnoMatchData, a2.mvnoMatchData);
    }

    @Test
    @SmallTest
    public void testFromString() throws Exception {
        String[] dunTypes = {"DUN"};
        String[] mmsTypes = {"mms", "*"};

        ApnSetting expectedApn;
        String testString;

        // A real-world v1 example string.
        testString = "Vodafone IT,web.omnitel.it,,,,,,,,,222,10,,DUN";
        expectedApn = new ApnSetting(
                -1, "22210", "Vodafone IT", "web.omnitel.it", "", "",
                "", "", "", "", "", 0, dunTypes, "IP", "IP", true, 0, 0,
                0, false, 0, 0, 0, 0, "", "");
        assertApnSettingEqual(expectedApn, ApnSetting.fromString(testString));

        // A v2 string.
        testString = "[ApnSettingV2] Name,apn,,,,,,,,,123,45,,mms|*,IPV6,IP,true,14";
        expectedApn = new ApnSetting(
                -1, "12345", "Name", "apn", "", "",
                "", "", "", "", "", 0, mmsTypes, "IPV6", "IP", true, 14, 0,
                0, false, 0, 0, 0, 0, "", "");
        assertApnSettingEqual(expectedApn, ApnSetting.fromString(testString));

        // A v2 string with spaces.
        testString = "[ApnSettingV2] Name,apn, ,,,,,,,,123,45,,mms|*,IPV6, IP,true,14";
        expectedApn = new ApnSetting(
                -1, "12345", "Name", "apn", "", "",
                "", "", "", "", "", 0, mmsTypes, "IPV6", "IP", true, 14, 0,
                0, false, 0, 0, 0, 0, "", "");
        assertApnSettingEqual(expectedApn, ApnSetting.fromString(testString));

        // A v3 string.
        testString = "[ApnSettingV3] Name,apn,,,,,,,,,123,45,,mms|*,IPV6,IP,true,14,,,,,,,spn,testspn";
        expectedApn = new ApnSetting(
                -1, "12345", "Name", "apn", "", "", "", "", "", "", "", 0, mmsTypes, "IPV6",
                "IP", true, 14, 0, 0, false, 0, 0, 0, 0, "spn", "testspn");
        assertApnSettingEqual(expectedApn, ApnSetting.fromString(testString));

        // Return no apn if insufficient fields given.
        testString = "[ApnSettingV3] Name,apn,,,,,,,,,123, 45,,mms|*";
        assertEquals(null, ApnSetting.fromString(testString));

        testString = "Name,apn,,,,,,,,,123, 45,";
        assertEquals(null, ApnSetting.fromString(testString));
    }

    @Test
    @SmallTest
    public void testArrayFromString() throws Exception {
        // Test a multiple v3 string.
        String testString =
                "[ApnSettingV3] Name,apn,,,,,,,,,123,45,,mms,IPV6,IP,true,14,,,,,,,spn,testspn";
        testString +=
                " ;[ApnSettingV3] Name1,apn1,,,,,,,,,123,46,,mms,IPV6,IP,true,12,,,,,,,gid,testGid";
        testString +=
                " ;[ApnSettingV3] Name1,apn2,,,,,,,,,123,46,,mms,IPV6,IP,true,12,,,,,,,,";
        List<ApnSetting> expectedApns = new ArrayList<ApnSetting>();
        expectedApns.add(new ApnSetting(
                -1, "12345", "Name", "apn", "", "", "", "", "", "", "", 0, new String[]{"mms"}, "IPV6",
                "IP", true, 14, 0, 0, false, 0, 0, 0, 0, "spn", "testspn"));
        expectedApns.add(new ApnSetting(
                -1, "12346", "Name1", "apn1", "", "", "", "", "", "", "", 0, new String[]{"mms"}, "IPV6",
                "IP", true, 12, 0, 0, false, 0, 0, 0, 0, "gid", "testGid"));
        expectedApns.add(new ApnSetting(
                -1, "12346", "Name1", "apn2", "", "", "", "", "", "", "", 0, new String[]{"mms"}, "IPV6",
                "IP", true, 12, 0, 0, false, 0, 0, 0, 0, "", ""));
        assertApnSettingsEqual(expectedApns, ApnSetting.arrayFromString(testString));
    }

    @Test
    @SmallTest
    public void testToString() throws Exception {
        String[] types = {"default", "*"};
        ApnSetting apn = new ApnSetting(
                99, "12345", "Name", "apn", "proxy", "port",
                "mmsc", "mmsproxy", "mmsport", "user", "password", 0,
                types, "IPV6", "IP", true, 14, 0, 0, false, 0, 0, 0, 0, "", "");
        String expected = "[ApnSettingV3] Name, 99, 12345, apn, proxy, " +
                "mmsc, mmsproxy, mmsport, port, 0, default | *, " +
                "IPV6, IP, true, 14, 8192, 0, false, 0, 0, 0, 0, , , false";
        assertEquals(expected, apn.toString());
    }

    @Test
    @SmallTest
    public void testIsMetered() throws Exception {
        mBundle.putStringArray(CarrierConfigManager.KEY_CARRIER_METERED_APN_TYPES_STRINGS,
                new String[]{PhoneConstants.APN_TYPE_DEFAULT, PhoneConstants.APN_TYPE_MMS});

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_DEFAULT}).
                isMetered(mContext, 1, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_DEFAULT, PhoneConstants.APN_TYPE_MMS}).
                isMetered(mContext, 1, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_MMS}).
                isMetered(mContext, 1, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_MMS, PhoneConstants.APN_TYPE_SUPL}).
                isMetered(mContext, 1, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_DEFAULT, PhoneConstants.APN_TYPE_DUN}).
                isMetered(mContext, 1, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_ALL}).
                isMetered(mContext, 1, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_FOTA, PhoneConstants.APN_TYPE_SUPL}).
                isMetered(mContext, 1, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_IA, PhoneConstants.APN_TYPE_CBS}).
                isMetered(mContext, 1, isRoaming));

        //reuse the cached result for subId 1
        assertTrue(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_DEFAULT,
                mContext, 1, isRoaming));
        assertTrue(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_MMS,
                mContext, 1, isRoaming));
        assertFalse(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_SUPL,
                mContext, 1, isRoaming));
        assertFalse(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_CBS,
                mContext, 1, isRoaming));
        assertFalse(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_DUN,
                mContext, 1, isRoaming));
        assertFalse(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_FOTA,
                mContext, 1, isRoaming));
        assertFalse(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_IA,
                mContext, 1, isRoaming));
        assertFalse(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_HIPRI,
                mContext, 1, isRoaming));
    }

    @Test
    @SmallTest
    public void testIsRoamingMetered() throws Exception {
        mBundle.putStringArray(CarrierConfigManager.KEY_CARRIER_METERED_ROAMING_APN_TYPES_STRINGS,
                new String[]{PhoneConstants.APN_TYPE_DEFAULT, PhoneConstants.APN_TYPE_MMS});
        isRoaming = true;

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_DEFAULT}).
                isMetered(mContext, 1, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_DEFAULT, PhoneConstants.APN_TYPE_MMS}).
                isMetered(mContext, 1, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_MMS}).
                isMetered(mContext, 1, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_MMS, PhoneConstants.APN_TYPE_SUPL}).
                isMetered(mContext, 1, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_DEFAULT, PhoneConstants.APN_TYPE_DUN}).
                isMetered(mContext, 1, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_ALL}).
                isMetered(mContext, 1, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_FOTA, PhoneConstants.APN_TYPE_SUPL}).
                isMetered(mContext, 1, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_IA, PhoneConstants.APN_TYPE_CBS}).
                isMetered(mContext, 1, isRoaming));
    }

    @Test
    @SmallTest
    public void testIsMeteredAnother() throws Exception {
        mBundle.putStringArray(CarrierConfigManager.KEY_CARRIER_METERED_APN_TYPES_STRINGS,
                new String[]{PhoneConstants.APN_TYPE_SUPL, PhoneConstants.APN_TYPE_CBS});

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_SUPL, PhoneConstants.APN_TYPE_CBS}).
                isMetered(mContext, 2, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_SUPL}).
                isMetered(mContext, 2, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_CBS}).
                isMetered(mContext, 2, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_FOTA, PhoneConstants.APN_TYPE_CBS}).
                isMetered(mContext, 2, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_SUPL, PhoneConstants.APN_TYPE_IA}).
                isMetered(mContext, 2, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_ALL}).
                isMetered(mContext, 2, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_DEFAULT, PhoneConstants.APN_TYPE_IMS}).
                isMetered(mContext, 2, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_IMS}).
                isMetered(mContext, 2, isRoaming));

    }

    @Test
    @SmallTest
    public void testIsRoamingMeteredAnother() throws Exception {
        mBundle.putStringArray(CarrierConfigManager.KEY_CARRIER_METERED_ROAMING_APN_TYPES_STRINGS,
                new String[]{PhoneConstants.APN_TYPE_SUPL, PhoneConstants.APN_TYPE_CBS});
        isRoaming = true;
        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_SUPL, PhoneConstants.APN_TYPE_CBS}).
                isMetered(mContext, 2, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_SUPL}).
                isMetered(mContext, 2, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_CBS}).
                isMetered(mContext, 2, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_FOTA, PhoneConstants.APN_TYPE_CBS}).
                isMetered(mContext, 2, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_SUPL, PhoneConstants.APN_TYPE_IA}).
                isMetered(mContext, 2, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_ALL}).
                isMetered(mContext, 2, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_DEFAULT, PhoneConstants.APN_TYPE_IMS}).
                isMetered(mContext, 2, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_IMS}).
                isMetered(mContext, 2, isRoaming));

        //reuse the cached result for subId 2
        assertTrue(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_SUPL,
                mContext, 2, isRoaming));
        assertTrue(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_CBS,
                mContext, 2, isRoaming));
        assertFalse(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_DEFAULT,
                mContext, 2, isRoaming));
        assertFalse(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_MMS,
                mContext, 2, isRoaming));
        assertFalse(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_DUN,
                mContext, 2, isRoaming));
        assertFalse(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_FOTA,
                mContext, 2, isRoaming));
        assertFalse(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_IA,
                mContext, 2, isRoaming));
        assertFalse(ApnSetting.isMeteredApnType(PhoneConstants.APN_TYPE_HIPRI,
                mContext, 2, isRoaming));

    }

    @Test
    @SmallTest
    public void testIsMeteredNothingCharged() throws Exception {
        mBundle.putStringArray(CarrierConfigManager.KEY_CARRIER_METERED_APN_TYPES_STRINGS,
                new String[]{});

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_IMS}).
                isMetered(mContext, 3, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_IMS, PhoneConstants.APN_TYPE_MMS}).
                isMetered(mContext, 3, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_DEFAULT, PhoneConstants.APN_TYPE_FOTA}).
                isMetered(mContext, 3, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_ALL}).
                isMetered(mContext, 3, isRoaming));
    }

    @Test
    @SmallTest
    public void testIsRoamingMeteredNothingCharged() throws Exception {
        mBundle.putStringArray(CarrierConfigManager.KEY_CARRIER_METERED_ROAMING_APN_TYPES_STRINGS,
                new String[]{});
        isRoaming = true;

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_IMS}).
                isMetered(mContext, 3, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_IMS, PhoneConstants.APN_TYPE_MMS}).
                isMetered(mContext, 3, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_DEFAULT, PhoneConstants.APN_TYPE_FOTA}).
                isMetered(mContext, 3, isRoaming));

        assertFalse(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_ALL}).
                isMetered(mContext, 3, isRoaming));
    }

    @Test
    @SmallTest
    public void testIsMeteredNothingFree() throws Exception {
        mBundle.putStringArray(CarrierConfigManager.KEY_CARRIER_METERED_APN_TYPES_STRINGS,
                new String[]{PhoneConstants.APN_TYPE_ALL});

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_ALL}).
                isMetered(mContext, 4, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_DEFAULT, PhoneConstants.APN_TYPE_MMS}).
                isMetered(mContext, 4, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_FOTA, PhoneConstants.APN_TYPE_CBS}).
                isMetered(mContext, 4, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_IA, PhoneConstants.APN_TYPE_DUN}).
                isMetered(mContext, 4, isRoaming));

    }

    @Test
    @SmallTest
    public void testIsRoamingMeteredNothingFree() throws Exception {
        mBundle.putStringArray(CarrierConfigManager.KEY_CARRIER_METERED_ROAMING_APN_TYPES_STRINGS,
                new String[]{PhoneConstants.APN_TYPE_ALL});
        isRoaming = true;

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_ALL}).
                isMetered(mContext, 4, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_DEFAULT, PhoneConstants.APN_TYPE_MMS}).
                isMetered(mContext, 4, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_FOTA, PhoneConstants.APN_TYPE_CBS}).
                isMetered(mContext, 4, isRoaming));

        assertTrue(createApnSetting(
                new String[]{PhoneConstants.APN_TYPE_IA, PhoneConstants.APN_TYPE_DUN}).
                isMetered(mContext, 4, isRoaming));

    }
}