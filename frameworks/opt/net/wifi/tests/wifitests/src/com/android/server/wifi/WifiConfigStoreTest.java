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
 * limitations under the License
 */

package com.android.server.wifi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import android.test.suitebuilder.annotation.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/**
 * Unit tests for {@link com.android.server.wifi.WifiConfigStore}.
 */
@SmallTest
public class WifiConfigStoreTest {
    private static final String KEY_SSID = "ssid";
    private static final String KEY_PSK = "psk";
    private static final String KEY_KEY_MGMT = "key_mgmt";
    private static final String KEY_PRIORITY = "priority";
    private static final String KEY_DISABLED = "disabled";
    private static final String KEY_ID_STR = "id_str";
    // This is not actually present as a key in the wpa_supplicant.conf file, but
    // is used in tests to conveniently access the configKey for a test network.
    private static final String CONFIG_KEY = "configKey";

    private static final HashMap<String, String> NETWORK_0_VARS = new HashMap<>();
    static {
        NETWORK_0_VARS.put(KEY_SSID, "\"TestNetwork0\"");
        NETWORK_0_VARS.put(KEY_KEY_MGMT, "NONE");
        NETWORK_0_VARS.put(KEY_PRIORITY, "2");
        NETWORK_0_VARS.put(KEY_ID_STR, ""
                + "\"%7B%22creatorUid%22%3A%221000%22%2C%22configKey%22%3A%22%5C%22"
                + "TestNetwork0%5C%22NONE%22%7D\"");
        NETWORK_0_VARS.put(CONFIG_KEY, "\"TestNetwork0\"NONE");
    }

    private static final HashMap<String, String> NETWORK_1_VARS = new HashMap<>();
    static {
        NETWORK_1_VARS.put(KEY_SSID, "\"Test Network 1\"");
        NETWORK_1_VARS.put(KEY_KEY_MGMT, "NONE");
        NETWORK_1_VARS.put(KEY_PRIORITY, "3");
        NETWORK_1_VARS.put(KEY_DISABLED, "1");
        NETWORK_1_VARS.put(KEY_ID_STR, ""
                + "\"%7B%22creatorUid%22%3A%221000%22%2C%22configKey%22%3A%22%5C%22"
                + "Test+Network+1%5C%22NONE%22%7D\"");
        NETWORK_1_VARS.put(CONFIG_KEY, "\"Test Network 1\"NONE");
    }

    private static final HashMap<String, String> NETWORK_2_VARS = new HashMap<>();
    static {
        NETWORK_2_VARS.put(KEY_SSID, "\"testNetwork2\"");
        NETWORK_2_VARS.put(KEY_KEY_MGMT, "NONE");
        NETWORK_2_VARS.put(KEY_PRIORITY, "4");
        NETWORK_2_VARS.put(KEY_DISABLED, "1");
        NETWORK_2_VARS.put(KEY_ID_STR, ""
                + "\"%7B%22creatorUid%22%3A%221000%22%2C%22configKey%22%3A%22%5C%22"
                + "testNetwork2%5C%22NONE%22%7D\"");
        NETWORK_2_VARS.put(CONFIG_KEY, "\"testNetwork2\"NONE");
    }

    private static final HashMap<String, String> NETWORK_3_VARS = new HashMap<>();
    static {
        NETWORK_3_VARS.put(KEY_SSID, "\"testwpa2psk\"");
        NETWORK_3_VARS.put(KEY_PSK, "blahblah");
        NETWORK_3_VARS.put(KEY_KEY_MGMT, "WPA-PSK");
        NETWORK_3_VARS.put(KEY_PRIORITY, "6");
        NETWORK_3_VARS.put(KEY_DISABLED, "1");
        NETWORK_3_VARS.put(KEY_ID_STR, ""
                + "\"%7B%22creatorUid%22%3A%221000%22%2C%22configKey%22%3A%22%5C%22"
                + "testwpa2psk%5C%22WPA_PSK%22%7D\"");
        NETWORK_3_VARS.put(CONFIG_KEY, "\"testwpa2psk\"WPA_PSK");
    }

    private static final ArrayList<HashMap<String, String>> NETWORK_VARS = new ArrayList<HashMap<String, String>>();
    static {
        NETWORK_VARS.add(NETWORK_0_VARS);
        NETWORK_VARS.add(NETWORK_1_VARS);
        NETWORK_VARS.add(NETWORK_2_VARS);
        NETWORK_VARS.add(NETWORK_3_VARS);
    }

    // Taken from wpa_supplicant.conf actual test device Some fields modified for privacy.
    private static final String TEST_WPA_SUPPLICANT_CONF = ""
            + "ctrl_interface=/data/misc/wifi/sockets\n"
            + "disable_scan_offload=1\n"
            + "driver_param=use_p2p_group_interface=1p2p_device=1\n"
            + "update_config=1\n"
            + "device_name=testdevice\n"
            + "manufacturer=TestManufacturer\n"
            + "model_name=Testxus\n"
            + "model_number=Testxus\n"
            + "serial_number=1ABCD12345678912\n"
            + "device_type=12-3456A456-7\n"
            + "config_methods=physical_display virtual_push_button\n"
            + "p2p_no_go_freq=5170-5740\n"
            + "pmf=1\n"
            + "external_sim=1\n"
            + "wowlan_triggers=any\n"
            + "p2p_search_delay=0\n"
            + "network={\n"
            + "        " + KEY_SSID + "=" + NETWORK_0_VARS.get(KEY_SSID) + "\n"
            + "        " + KEY_KEY_MGMT + "=" + NETWORK_0_VARS.get(KEY_KEY_MGMT) + "\n"
            + "        " + KEY_PRIORITY + "=" + NETWORK_0_VARS.get(KEY_PRIORITY) + "\n"
            + "        " + KEY_ID_STR + "=" + NETWORK_0_VARS.get(KEY_ID_STR) + "\n"
            + "}\n"
            + "\n"
            + "network={\n"
            + "        " + KEY_SSID + "=" + NETWORK_1_VARS.get(KEY_SSID) + "\n"
            + "        " + KEY_KEY_MGMT + "=" + NETWORK_1_VARS.get(KEY_KEY_MGMT) + "\n"
            + "        " + KEY_PRIORITY + "=" + NETWORK_1_VARS.get(KEY_PRIORITY) + "\n"
            + "        " + KEY_DISABLED + "=" + NETWORK_1_VARS.get(KEY_DISABLED) + "\n"
            + "        " + KEY_ID_STR + "=" + NETWORK_1_VARS.get(KEY_ID_STR) + "\n"
            + "}\n"
            + "\n"
            + "network={\n"
            + "        " + KEY_SSID + "=" + NETWORK_2_VARS.get(KEY_SSID) + "\n"
            + "        " + KEY_KEY_MGMT + "=" + NETWORK_2_VARS.get(KEY_KEY_MGMT) + "\n"
            + "        " + KEY_PRIORITY + "=" + NETWORK_2_VARS.get(KEY_PRIORITY) + "\n"
            + "        " + KEY_DISABLED + "=" + NETWORK_2_VARS.get(KEY_DISABLED) + "\n"
            + "        " + KEY_ID_STR + "=" + NETWORK_2_VARS.get(KEY_ID_STR) + "\n"
            + "}\n"
            + "\n"
            + "network={\n"
            + "        " + KEY_SSID + "=" + NETWORK_3_VARS.get(KEY_SSID) + "\n"
            + "        " + KEY_PSK + "=" + NETWORK_3_VARS.get(KEY_PSK) + "\n"
            + "        " + KEY_KEY_MGMT + "=" + NETWORK_3_VARS.get(KEY_KEY_MGMT) + "\n"
            + "        " + KEY_PRIORITY + "=" + NETWORK_3_VARS.get(KEY_PRIORITY) + "\n"
            + "        " + KEY_DISABLED + "=" + NETWORK_3_VARS.get(KEY_DISABLED) + "\n"
            + "        " + KEY_ID_STR + "=" + NETWORK_3_VARS.get(KEY_ID_STR) + "\n"
            + "}\n";

    @Mock private WifiNative mWifiNative;
    private MockKeyStore mMockKeyStore;
    private WifiConfigStore mWifiConfigStore;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        mMockKeyStore = new MockKeyStore();
        mWifiConfigStore = new WifiConfigStore(mWifiNative, mMockKeyStore.createMock(), null,
                false, true);
    }

    /**
     * Verifies that readNetworkVariableFromSupplicantFile() properly reads network variables from a
     * wpa_supplicant.conf file.
     */
    @Test
    public void readNetworkVariableFromSupplicantFile() throws Exception {
        Map<String, String> ssidResults = readNetworkVariableFromSupplicantFile(KEY_SSID);
        assertEquals(NETWORK_VARS.size(), ssidResults.size());
        for (HashMap<String, String> single_network_vars : NETWORK_VARS) {
            assertEquals(ssidResults.get(single_network_vars.get(CONFIG_KEY)),
                    single_network_vars.get(KEY_SSID));
        }

        Map<String, String> pskResults = readNetworkVariableFromSupplicantFile(KEY_PSK);
        // Only network 3 is secured with a password.
        assertEquals(1, pskResults.size());
        assertEquals(pskResults.get(NETWORK_3_VARS.get(CONFIG_KEY)),
                    NETWORK_3_VARS.get(KEY_PSK));

        Map<String, String> keyMgmtResults = readNetworkVariableFromSupplicantFile(KEY_KEY_MGMT);
        assertEquals(NETWORK_VARS.size(), keyMgmtResults.size());
        for (HashMap<String, String> single_network_vars : NETWORK_VARS) {
            assertEquals(keyMgmtResults.get(single_network_vars.get(CONFIG_KEY)),
                    single_network_vars.get(KEY_KEY_MGMT));
        }

        Map<String, String> priorityResults = readNetworkVariableFromSupplicantFile(KEY_PRIORITY);
        assertEquals(NETWORK_VARS.size(), priorityResults.size());
        for (HashMap<String, String> single_network_vars : NETWORK_VARS) {
            assertEquals(priorityResults.get(single_network_vars.get(CONFIG_KEY)),
                    single_network_vars.get(KEY_PRIORITY));
        }

        Map<String, String> disabledResults = readNetworkVariableFromSupplicantFile(KEY_DISABLED);
        // All but network 0 are disabled.
        assertEquals(NETWORK_VARS.size() - 1, disabledResults.size());
        for (int i = 1; i < NETWORK_VARS.size(); ++i) {
            assertEquals(disabledResults.get(NETWORK_VARS.get(i).get(CONFIG_KEY)),
                    NETWORK_VARS.get(i).get(KEY_DISABLED));
        }

        Map<String, String> idStrResults = readNetworkVariableFromSupplicantFile(KEY_ID_STR);
        assertEquals(NETWORK_VARS.size(), idStrResults.size());
        for (HashMap<String, String> single_network_vars : NETWORK_VARS) {
            assertEquals(idStrResults.get(single_network_vars.get(CONFIG_KEY)),
                    single_network_vars.get(KEY_ID_STR));
        }
    }

    /**
     * Inject |TEST_WPA_SUPPLICANT_CONF| via the helper method readNetworkVariablesFromReader().
     */
    public Map<String, String> readNetworkVariableFromSupplicantFile(String key) throws Exception {
        Map<String, String> result = new HashMap<>();
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new StringReader(TEST_WPA_SUPPLICANT_CONF));
            result = mWifiConfigStore.readNetworkVariablesFromReader(reader, key);
        } catch (IOException e) {
            fail("Error reading test supplicant conf string");
        } finally {
            try {
                if (reader != null) {
                    reader.close();
                }
            } catch (IOException e) {
                // Just ignore if we can't close the reader.
            }
        }
        return result;
    }
}
