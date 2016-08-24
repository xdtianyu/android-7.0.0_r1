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

package com.android.server.wifi;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.test.suitebuilder.annotation.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

/**
 * Unit tests for {@link com.android.server.wifi.WifiCountryCode}.
 */
@SmallTest
public class WifiCountryCodeTest {

    private static final String TAG = "WifiCountryCodeTest";
    private String mDefaultCountryCode = "US";
    private String mTelephonyCountryCode = "JP";
    private String mPersistCountryCode = "";
    private boolean mRevertCountryCodeOnCellularLoss = true;
    @Mock WifiNative mWifiNative;
    private WifiCountryCode mWifiCountryCode;

    /**
     * Setup test.
     */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        when(mWifiNative.setCountryCode(anyString())).thenReturn(true);

        mWifiCountryCode = new WifiCountryCode(
                mWifiNative,
                mDefaultCountryCode,
                mPersistCountryCode,
                mRevertCountryCodeOnCellularLoss);
    }

    /**
     * Test if we do not receive country code from Telephony.
     * @throws Exception
     */
    @Test
    public void useDefaultCountryCode() throws Exception {
        // Supplicant started.
        mWifiCountryCode.setReadyForChange(true);
        // Wifi get L2 connected.
        mWifiCountryCode.setReadyForChange(false);
        verify(mWifiNative).setCountryCode(anyString());
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCurrentCountryCode());
    }

    /**
     * Test if we receive country code from Telephony before supplicant starts.
     * @throws Exception
     */
    @Test
    public void useTelephonyCountryCode() throws Exception {
        mWifiCountryCode.setCountryCode(mTelephonyCountryCode, false);
        assertEquals(null, mWifiCountryCode.getCurrentCountryCode());
        // Supplicant started.
        mWifiCountryCode.setReadyForChange(true);
        // Wifi get L2 connected.
        mWifiCountryCode.setReadyForChange(false);
        verify(mWifiNative).setCountryCode(anyString());
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCurrentCountryCode());
    }

    /**
     * Test if we receive country code from Telephony after supplicant starts.
     * @throws Exception
     */
    @Test
    public void setTelephonyCountryCodeAfterSupplicantStarts() throws Exception {
        // Supplicant starts.
        mWifiCountryCode.setReadyForChange(true);
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCurrentCountryCode());
        // Telephony country code arrives.
        mWifiCountryCode.setCountryCode(mTelephonyCountryCode, false);
        // Wifi get L2 connected.
        mWifiCountryCode.setReadyForChange(false);
        verify(mWifiNative, times(2)).setCountryCode(anyString());
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCurrentCountryCode());
    }

    /**
     * Test if we receive country code from Telephony after we get L2 connected.
     * @throws Exception
     */
    @Test
    public void setTelephonyCountryCodeAfterL2Connected() throws Exception {
        // Supplicant starts.
        mWifiCountryCode.setReadyForChange(true);
        // Wifi get L2 connected.
        mWifiCountryCode.setReadyForChange(false);
        // Telephony country code arrives.
        mWifiCountryCode.setCountryCode(mTelephonyCountryCode, false);
        // Telephony coutry code won't be applied at this time.
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCurrentCountryCode());
        mWifiCountryCode.setReadyForChange(true);
        // Telephony coutry is applied after supplicant is ready.
        verify(mWifiNative, times(2)).setCountryCode(anyString());
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCurrentCountryCode());
    }

    /**
     * Test if we can reset the country code upon sim card is removed.
     * @throws Exception
     */
    @Test
    public void resetCountryCodeWhenSIMCardRemoved() throws Exception {
        mWifiCountryCode.setCountryCode(mTelephonyCountryCode, false);
        // Supplicant started.
        mWifiCountryCode.setReadyForChange(true);
        // Wifi get L2 connected.
        mWifiCountryCode.setReadyForChange(false);
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCurrentCountryCode());
        // SIM card is removed.
        mWifiCountryCode.simCardRemoved();
        // Country code restting is not applied yet.
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCurrentCountryCode());
        mWifiCountryCode.setReadyForChange(true);
        // Country code restting is applied when supplicant is ready.
        verify(mWifiNative, times(2)).setCountryCode(anyString());
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCurrentCountryCode());
    }

    /**
     * Test if we can reset the country code upon airplane mode is enabled.
     * @throws Exception
     */
    @Test
    public void resetCountryCodeWhenAirplaneModeEnabled() throws Exception {
        mWifiCountryCode.setCountryCode(mTelephonyCountryCode, false);
        // Supplicant started.
        mWifiCountryCode.setReadyForChange(true);
        // Wifi get L2 connected.
        mWifiCountryCode.setReadyForChange(false);
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCurrentCountryCode());
        // Airplane mode is enabled.
        mWifiCountryCode.simCardRemoved();
        // Country code restting is not applied yet.
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCurrentCountryCode());
        mWifiCountryCode.setReadyForChange(true);
        // Country code restting is applied when supplicant is ready.
        verify(mWifiNative, times(2)).setCountryCode(anyString());
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCurrentCountryCode());
    }

    /**
     * Test if we will set the persistent country code if it is not empty.
     * @throws Exception
     */
    @Test
    public void usePersistentCountryCode() throws Exception {
        String persistentCountryCode = "CH";
        mWifiCountryCode = new WifiCountryCode(
                mWifiNative,
                mDefaultCountryCode,
                persistentCountryCode,
                mRevertCountryCodeOnCellularLoss);
        // Supplicant started.
        mWifiCountryCode.setReadyForChange(true);
        // Wifi get L2 connected.
        mWifiCountryCode.setReadyForChange(false);
        verify(mWifiNative).setCountryCode(anyString());
        assertEquals(persistentCountryCode, mWifiCountryCode.getCurrentCountryCode());
    }
}
