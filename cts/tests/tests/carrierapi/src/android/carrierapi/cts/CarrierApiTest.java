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

package android.carrierapi.cts;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.net.ConnectivityManager;
import android.os.PersistableBundle;
import android.telephony.CarrierConfigManager;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.test.AndroidTestCase;
import android.util.Log;

import com.android.internal.telephony.uicc.IccUtils;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class CarrierApiTest extends AndroidTestCase {
    private static final String TAG = "CarrierApiTest";
    private TelephonyManager mTelephonyManager;
    private PackageManager mPackageManager;
    private boolean hasCellular;
    private String selfPackageName;
    private String selfCertHash;

    private static final String FiDevCert = "24EB92CBB156B280FA4E1429A6ECEEB6E5C1BFE4";

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTelephonyManager = (TelephonyManager)
                getContext().getSystemService(Context.TELEPHONY_SERVICE);
        mPackageManager = getContext().getPackageManager();
        selfPackageName = getContext().getPackageName();
        selfCertHash = getCertHash(selfPackageName);
        hasCellular = hasCellular();
        if (!hasCellular) {
            Log.e(TAG, "No cellular support, all tests will be skipped.");
        }
    }

    /**
     * Checks whether the cellular stack should be running on this device.
     */
    private boolean hasCellular() {
        ConnectivityManager mgr =
                (ConnectivityManager) getContext().getSystemService(Context.CONNECTIVITY_SERVICE);
        return mgr.isNetworkSupported(ConnectivityManager.TYPE_MOBILE);
    }

    private boolean isSimCardPresent() {
        return mTelephonyManager.getPhoneType() != TelephonyManager.PHONE_TYPE_NONE &&
                mTelephonyManager.getSimState() != TelephonyManager.SIM_STATE_ABSENT;
    }

    private String getCertHash(String pkgName) {
        try {
            PackageInfo pInfo = mPackageManager.getPackageInfo(pkgName,
                    PackageManager.GET_SIGNATURES | PackageManager.GET_DISABLED_UNTIL_USED_COMPONENTS);
            MessageDigest md = MessageDigest.getInstance("SHA-1");
            return IccUtils.bytesToHexString(md.digest(pInfo.signatures[0].toByteArray()));
        } catch (PackageManager.NameNotFoundException ex) {
            Log.e(TAG, pkgName + " not found", ex);
        } catch (NoSuchAlgorithmException ex) {
            Log.e(TAG, "Algorithm SHA1 is not found.");
        }
        return "";
    }

    private void failMessage() {
        if (FiDevCert.equalsIgnoreCase(selfCertHash)) {
            fail("This test requires a Project Fi SIM card.");
        } else {
            fail("This test requires a SIM card with carrier privilege rule on it.\n" +
                 "Cert hash: " + selfCertHash + "\n" +
                 "Visit https://source.android.com/devices/tech/config/uicc.html");
        }
    }

    public void testSimCardPresent() {
        if (!hasCellular) return;
        assertTrue("This test requires SIM card.", isSimCardPresent());
    }

    public void testHasCarrierPrivileges() {
        if (!hasCellular) return;
        if (!mTelephonyManager.hasCarrierPrivileges()) {
            failMessage();
        }
    }

}
