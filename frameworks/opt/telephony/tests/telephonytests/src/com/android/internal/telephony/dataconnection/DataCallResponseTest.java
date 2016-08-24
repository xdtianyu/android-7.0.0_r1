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

package com.android.internal.telephony.dataconnection;

import android.net.LinkProperties;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.internal.telephony.TelephonyTest;
import com.android.internal.telephony.dataconnection.DataCallResponse.SetupResult;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class DataCallResponseTest extends TelephonyTest {

    DataCallResponse mDcResponse;

    @Before
    public void setUp() throws Exception {
        super.setUp(getClass().getSimpleName());
        mDcResponse = new DataCallResponse();
        mDcResponse.version = 11;
        mDcResponse.status = 0;
        mDcResponse.suggestedRetryTime = -1;
        mDcResponse.cid = 1;
        mDcResponse.active = 2;
        mDcResponse.type = "IP";
        mDcResponse.ifname = "rmnet_data7";
        mDcResponse.mtu = 1440;
        mDcResponse.addresses = new String[]{"12.34.56.78"};
        mDcResponse.dnses = new String[]{"98.76.54.32"};
        mDcResponse.gateways = new String[]{"11.22.33.44"};
        mDcResponse.pcscf = new String[]{};
    }

    @After
    public void tearDown() throws Exception {
        super.tearDown();
    }

    @Test
    @SmallTest
    public void testSetLinkProperties() throws Exception {
        LinkProperties linkProperties = new LinkProperties();
        assertEquals(SetupResult.SUCCESS,
                mDcResponse.setLinkProperties(linkProperties, true));
        logd(linkProperties.toString());
        assertEquals(mDcResponse.ifname, linkProperties.getInterfaceName());
        assertEquals(mDcResponse.addresses.length, linkProperties.getAddresses().size());
        for (int i = 0; i < mDcResponse.addresses.length; ++i) {
            assertEquals(mDcResponse.addresses[i],
                    linkProperties.getLinkAddresses().get(i).getAddress().getHostAddress());
        }

        assertEquals(mDcResponse.dnses.length, linkProperties.getDnsServers().size());
        for (int i = 0; i < mDcResponse.dnses.length; ++i) {
            assertEquals("i = " + i, mDcResponse.dnses[i],
                    linkProperties.getDnsServers().get(i).getHostAddress());
        }

        assertEquals(mDcResponse.gateways.length, linkProperties.getRoutes().size());
        for (int i = 0; i < mDcResponse.gateways.length; ++i) {
            assertEquals("i = " + i, mDcResponse.gateways[i],
                    linkProperties.getRoutes().get(i).getGateway().getHostAddress());
        }

        assertEquals(mDcResponse.mtu, linkProperties.getMtu());
    }

    @Test
    @SmallTest
    public void testSetLinkPropertiesInvalidAddress() throws Exception {

        mDcResponse.addresses = new String[]{"224.224.224.224"};

        LinkProperties linkProperties = new LinkProperties();
        assertEquals(SetupResult.ERR_UnacceptableParameter,
                mDcResponse.setLinkProperties(linkProperties, true));
    }
}