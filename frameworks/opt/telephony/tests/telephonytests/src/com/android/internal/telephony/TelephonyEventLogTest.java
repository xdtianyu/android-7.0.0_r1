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

package com.android.internal.telephony;

import com.android.ims.ImsConfig;
import com.android.ims.ImsReasonInfo;

import android.net.ConnectivityMetricsEvent;
import android.net.ConnectivityMetricsLogger;
import android.net.IConnectivityMetricsLogger;
import android.os.Bundle;
import android.os.RemoteException;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.ArrayMap;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;

public class TelephonyEventLogTest extends TelephonyTest {
    private static final String TAG = "TelephonyEventLogTest";

    @Mock
    private IConnectivityMetricsLogger.Stub mConnectivityMetricsLogger;

    private TelephonyEventLog mEventLog;

    private static final class TelephonyEventMatcher extends BaseMatcher<ConnectivityMetricsEvent> {
        int mEventTag;
        ArrayMap<String, Object> mMap = null;

        public TelephonyEventMatcher(int eventTag, ArrayMap<String, Object> m) {
            mEventTag = eventTag;
            mMap = m;
        }

        @Override
        public boolean matches(Object item) {
            ConnectivityMetricsEvent e = (ConnectivityMetricsEvent) item;

            if (e.componentTag != ConnectivityMetricsLogger.COMPONENT_TAG_TELEPHONY) {
                logd("Component Tag, actual: " + e.componentTag);
                return false;
            }

            if (e.eventTag != mEventTag) {
                logd("Component Tag, expected: " + mEventTag + ", actual: " + e.eventTag);
                return false;
            }

            Bundle b = (Bundle) e.data;

            // compare only values stored in the map
            for (int i=0; i < mMap.size(); i++) {
                String key = mMap.keyAt(i);
                Object value = mMap.valueAt(i);
                if (!value.equals(b.get(key))) {
                    logd("key: " + key + ", expected: " + value + ", actual: " + b.get(key));
                    return false;
                }
            }

            return true;
        }

        @Override
        public void describeTo(Description description) {
            description.appendText(" tag: " + mEventTag);
            description.appendText(", data: " + mMap.toString());
        }
    }

    @Before
    public void setUp() throws Exception {
        logd("setUp start");
        super.setUp(TAG);

        doReturn(mConnectivityMetricsLogger)
                .when(mConnMetLoggerBinder)
                .queryLocalInterface(anyString());

        mEventLog = new TelephonyEventLog(0);
    }

    @After
    public void tearDown() throws Exception {
        mEventLog = null;
        super.tearDown();
    }

    @Test @SmallTest
    public void testWriteServiceStateChanged() {
        ServiceState serviceState = new ServiceState();
        serviceState.setVoiceRegState(ServiceState.STATE_IN_SERVICE);
        serviceState.setDataRegState(ServiceState.STATE_IN_SERVICE);
        serviceState.setVoiceRoamingType(ServiceState.ROAMING_TYPE_NOT_ROAMING);
        serviceState.setDataRoamingType(ServiceState.ROAMING_TYPE_NOT_ROAMING);
        serviceState.setVoiceOperatorName("Test Voice Long", "TestVoice", "12345");
        serviceState.setDataOperatorName("Test Date Long", "TestData", "67890");
        serviceState.setRilVoiceRadioTechnology(ServiceState.RIL_RADIO_TECHNOLOGY_LTE);
        serviceState.setRilDataRadioTechnology(ServiceState.RIL_RADIO_TECHNOLOGY_LTE);

        mEventLog.writeServiceStateChanged(serviceState);

        ArrayMap<String, Object> m = new ArrayMap<>();
        m.put(TelephonyEventLog.SERVICE_STATE_VOICE_REG_STATE, ServiceState.STATE_IN_SERVICE);
        m.put(TelephonyEventLog.SERVICE_STATE_DATA_REG_STATE, ServiceState.STATE_IN_SERVICE);
        m.put(TelephonyEventLog.SERVICE_STATE_VOICE_ROAMING_TYPE,
                ServiceState.ROAMING_TYPE_NOT_ROAMING);
        m.put(TelephonyEventLog.SERVICE_STATE_DATA_ROAMING_TYPE,
                ServiceState.ROAMING_TYPE_NOT_ROAMING);
        //m.put(TelephonyEventLog.SERVICE_STATE_VOICE_ALPHA_LONG, "Test Voice Long");
        m.put(TelephonyEventLog.SERVICE_STATE_VOICE_ALPNA_SHORT, "TestVoice");
        m.put(TelephonyEventLog.SERVICE_STATE_VOICE_NUMERIC, "12345");
        //m.put(TelephonyEventLog.SERVICE_STATE_DATA_ALPHA_LONG, "Test Date Long");
        m.put(TelephonyEventLog.SERVICE_STATE_DATA_ALPNA_SHORT, "TestData");
        m.put(TelephonyEventLog.SERVICE_STATE_DATA_NUMERIC, "67890");
        m.put(TelephonyEventLog.SERVICE_STATE_VOICE_RAT, ServiceState.RIL_RADIO_TECHNOLOGY_LTE);
        m.put(TelephonyEventLog.SERVICE_STATE_DATA_RAT, ServiceState.RIL_RADIO_TECHNOLOGY_LTE);

        m.put(TelephonyEventLog.DATA_KEY_PHONE_ID, 0);
        m.put(TelephonyEventLog.DATA_KEY_PARAM1, -1);
        m.put(TelephonyEventLog.DATA_KEY_PARAM2, -1);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SERVICE_STATE, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }
    }

    @Test @SmallTest
    public void testWriteSetAirplaneMode() {
        mEventLog.writeSetAirplaneMode(true);

        ArrayMap<String, Object> m = new ArrayMap<>();
        m.put(TelephonyEventLog.DATA_KEY_PHONE_ID, 0);
        m.put(TelephonyEventLog.DATA_KEY_PARAM1, TelephonyEventLog.SETTING_AIRPLANE_MODE);
        m.put(TelephonyEventLog.DATA_KEY_PARAM2, 1);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }

        mEventLog.writeSetAirplaneMode(false);

        m.put(TelephonyEventLog.DATA_KEY_PARAM2, 0);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }
    }

    @Test @SmallTest
    public void testWriteSetCellDataEnabled() {
        mEventLog.writeSetCellDataEnabled(true);

        ArrayMap<String, Object> m = new ArrayMap<>();
        m.put(TelephonyEventLog.DATA_KEY_PHONE_ID, 0);
        m.put(TelephonyEventLog.DATA_KEY_PARAM1, TelephonyEventLog.SETTING_CELL_DATA_ENABLED);
        m.put(TelephonyEventLog.DATA_KEY_PARAM2, 1);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }

        mEventLog.writeSetCellDataEnabled(false);

        m.put(TelephonyEventLog.DATA_KEY_PARAM2, 0);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }
    }

    @Test @SmallTest
    public void testWriteSetDataRoamingEnabled() {
        mEventLog.writeSetDataRoamingEnabled(true);

        ArrayMap<String, Object> m = new ArrayMap<>();
        m.put(TelephonyEventLog.DATA_KEY_PHONE_ID, 0);
        m.put(TelephonyEventLog.DATA_KEY_PARAM1, TelephonyEventLog.SETTING_DATA_ROAMING_ENABLED);
        m.put(TelephonyEventLog.DATA_KEY_PARAM2, 1);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }

        mEventLog.writeSetDataRoamingEnabled(false);

        m.put(TelephonyEventLog.DATA_KEY_PARAM2, 0);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }
    }

    @Test @SmallTest
    public void testWriteSetPreferredNetworkType() {
        mEventLog.writeSetPreferredNetworkType(RILConstants.NETWORK_MODE_GLOBAL);

        ArrayMap<String, Object> m = new ArrayMap<>();
        m.put(TelephonyEventLog.DATA_KEY_PHONE_ID, 0);
        m.put(TelephonyEventLog.DATA_KEY_PARAM1, TelephonyEventLog.SETTING_PREFERRED_NETWORK_MODE);
        m.put(TelephonyEventLog.DATA_KEY_PARAM2, RILConstants.NETWORK_MODE_GLOBAL);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }
    }

    @Test @SmallTest
    public void testWriteSetWifiEnabled() {
        mEventLog.writeSetWifiEnabled(true);

        ArrayMap<String, Object> m = new ArrayMap<>();
        m.put(TelephonyEventLog.DATA_KEY_PHONE_ID, 0);
        m.put(TelephonyEventLog.DATA_KEY_PARAM1, TelephonyEventLog.SETTING_WIFI_ENABLED);
        m.put(TelephonyEventLog.DATA_KEY_PARAM2, 1);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }

        mEventLog.writeSetWifiEnabled(false);

        m.put(TelephonyEventLog.DATA_KEY_PARAM2, 0);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }
    }

    @Test @SmallTest
    public void testWriteSetWfcMode() {
        mEventLog.writeSetWfcMode(ImsConfig.WfcModeFeatureValueConstants.CELLULAR_PREFERRED);

        ArrayMap<String, Object> m = new ArrayMap<>();
        m.put(TelephonyEventLog.DATA_KEY_PHONE_ID, 0);
        m.put(TelephonyEventLog.DATA_KEY_PARAM1, TelephonyEventLog.SETTING_WFC_MODE);
        m.put(TelephonyEventLog.DATA_KEY_PARAM2, ImsConfig.WfcModeFeatureValueConstants.CELLULAR_PREFERRED);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }
    }

    @Test @SmallTest
    public void testWriteImsSetFeatureValue() {
        mEventLog.writeImsSetFeatureValue(ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE,
                TelephonyManager.NETWORK_TYPE_LTE, 1, ImsConfig.OperationStatusConstants.SUCCESS);

        ArrayMap<String, Object> m = new ArrayMap<>();
        m.put(TelephonyEventLog.DATA_KEY_PHONE_ID, 0);
        m.put(TelephonyEventLog.DATA_KEY_PARAM1, TelephonyEventLog.SETTING_VO_LTE_ENABLED);
        m.put(TelephonyEventLog.DATA_KEY_PARAM2, 1);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }

        mEventLog.writeImsSetFeatureValue(ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI,
                TelephonyManager.NETWORK_TYPE_IWLAN, 1, ImsConfig.OperationStatusConstants.SUCCESS);

        m.put(TelephonyEventLog.DATA_KEY_PARAM1, TelephonyEventLog.SETTING_VO_WIFI_ENABLED);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }

        mEventLog.writeImsSetFeatureValue(ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE,
                TelephonyManager.NETWORK_TYPE_LTE, 1, ImsConfig.OperationStatusConstants.SUCCESS);

        m.put(TelephonyEventLog.DATA_KEY_PARAM1, TelephonyEventLog.SETTING_VI_LTE_ENABLED);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }

        mEventLog.writeImsSetFeatureValue(ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI,
                TelephonyManager.NETWORK_TYPE_IWLAN, 1, ImsConfig.OperationStatusConstants.SUCCESS);

        m.put(TelephonyEventLog.DATA_KEY_PARAM1, TelephonyEventLog.SETTING_VI_WIFI_ENABLED);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(TelephonyEventLog.TAG_SETTINGS, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }
    }

    @Test @SmallTest
    public void testWriteImsConnectionState() {
        mEventLog.writeOnImsConnectionState(
                TelephonyEventLog.IMS_CONNECTION_STATE_CONNECTED, null);

        ArrayMap<String, Object> m = new ArrayMap<>();
        m.put(TelephonyEventLog.DATA_KEY_PHONE_ID, 0);
        m.put(TelephonyEventLog.DATA_KEY_PARAM1, TelephonyEventLog.IMS_CONNECTION_STATE_CONNECTED);
        m.put(TelephonyEventLog.DATA_KEY_PARAM2, -1);

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(
                            TelephonyEventLog.TAG_IMS_CONNECTION_STATE, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }

        mEventLog.writeOnImsConnectionState(
                TelephonyEventLog.IMS_CONNECTION_STATE_DISCONNECTED,
                new ImsReasonInfo(1, 2, "test"));

        m.put(TelephonyEventLog.DATA_KEY_PARAM1,
                TelephonyEventLog.IMS_CONNECTION_STATE_DISCONNECTED);
        m.put(TelephonyEventLog.DATA_KEY_REASONINFO_CODE, 1);
        m.put(TelephonyEventLog.DATA_KEY_REASONINFO_EXTRA_CODE, 2);
        m.put(TelephonyEventLog.DATA_KEY_REASONINFO_EXTRA_MESSAGE, "test");

        try {
            verify(mConnectivityMetricsLogger).logEvent(
                    argThat(new TelephonyEventMatcher(
                            TelephonyEventLog.TAG_IMS_CONNECTION_STATE, m)));
        } catch (RemoteException e) {
            fail(e.toString());
        }
    }
}