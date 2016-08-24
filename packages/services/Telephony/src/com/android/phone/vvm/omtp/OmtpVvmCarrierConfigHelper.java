/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.phone.vvm.omtp;

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.PersistableBundle;
import android.telephony.CarrierConfigManager;
import android.telephony.SmsManager;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.phone.vvm.omtp.sms.OmtpCvvmMessageSender;
import com.android.phone.vvm.omtp.sms.OmtpMessageSender;
import com.android.phone.vvm.omtp.sms.OmtpStandardMessageSender;

/**
 * Handle activation and deactivation of a visual voicemail source. This class is necessary to
 * retrieve carrier vvm configuration details before sending the appropriate texts.
 */
public class OmtpVvmCarrierConfigHelper {

    private static final String TAG = "OmtpVvmCarrierCfgHlpr";
    private Context mContext;
    private int mSubId;
    private PersistableBundle mCarrierConfig;
    private String mVvmType;

    public OmtpVvmCarrierConfigHelper(Context context, int subId) {
        mContext = context;
        mSubId = subId;
        mCarrierConfig = getCarrierConfig();
        mVvmType = getVvmType();
    }

    public String getVvmType() {
        if (mCarrierConfig == null) {
            return null;
        }

        return mCarrierConfig.getString(
                CarrierConfigManager.KEY_VVM_TYPE_STRING, null);
    }

    public String getCarrierVvmPackageName() {
        if (mCarrierConfig == null) {
            return null;
        }

        return mCarrierConfig.getString(
                CarrierConfigManager.KEY_CARRIER_VVM_PACKAGE_NAME_STRING, null);
    }

    public boolean isOmtpVvmType() {
        return (TelephonyManager.VVM_TYPE_OMTP.equals(mVvmType) ||
                TelephonyManager.VVM_TYPE_CVVM.equals(mVvmType));
    }

    /**
     * For checking upon sim insertion whether visual voicemail should be enabled. This method does
     * so by checking if the carrier's voicemail app is installed.
     */
    public boolean isEnabledByDefault() {
        String packageName = mCarrierConfig.getString(
                CarrierConfigManager.KEY_CARRIER_VVM_PACKAGE_NAME_STRING);
        if (packageName == null) {
            return true;
        }
        try {
            mContext.getPackageManager().getPackageInfo(packageName, 0);
            return false;
        } catch (NameNotFoundException e) {
            return true;
        }
    }

    public boolean isCellularDataRequired() {
        if (mCarrierConfig == null) {
            return false;
        }
        return mCarrierConfig
                .getBoolean(CarrierConfigManager.KEY_VVM_CELLULAR_DATA_REQUIRED_BOOL);
    }

    public boolean isPrefetchEnabled() {
        if (mCarrierConfig == null) {
            return false;
        }
        return mCarrierConfig
                .getBoolean(CarrierConfigManager.KEY_VVM_PREFETCH_BOOL);
    }

    public void startActivation() {
        OmtpMessageSender messageSender = getMessageSender();
        if (messageSender != null) {
            Log.i(TAG, "Requesting VVM activation for subId: " + mSubId);
            messageSender.requestVvmActivation(null);
        }
    }

    public void startDeactivation() {
        OmtpMessageSender messageSender = getMessageSender();
        if (messageSender != null) {
            Log.i(TAG, "Requesting VVM deactivation for subId: " + mSubId);
            messageSender.requestVvmDeactivation(null);
        }
    }

    private PersistableBundle getCarrierConfig() {
        if (!SubscriptionManager.isValidSubscriptionId(mSubId)) {
            Log.w(TAG, "Invalid subscriptionId or subscriptionId not provided in intent.");
            return null;
        }

        CarrierConfigManager carrierConfigManager = (CarrierConfigManager)
                mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        if (carrierConfigManager == null) {
            Log.w(TAG, "No carrier config service found.");
            return null;
        }

        return carrierConfigManager.getConfigForSubId(mSubId);
    }

    private OmtpMessageSender getMessageSender() {
        if (mCarrierConfig == null) {
            Log.w(TAG, "Empty carrier config.");
            return null;
        }

        int applicationPort = mCarrierConfig.getInt(
                CarrierConfigManager.KEY_VVM_PORT_NUMBER_INT, 0);
        String destinationNumber = mCarrierConfig.getString(
                CarrierConfigManager.KEY_VVM_DESTINATION_NUMBER_STRING);
        if (TextUtils.isEmpty(destinationNumber)) {
            Log.w(TAG, "No destination number for this carrier.");
            return null;
        }

        OmtpMessageSender messageSender = null;
        SmsManager smsManager = SmsManager.getSmsManagerForSubscriptionId(mSubId);
        switch (mVvmType) {
            case TelephonyManager.VVM_TYPE_OMTP:
                messageSender = new OmtpStandardMessageSender(smsManager, (short) applicationPort,
                        destinationNumber, null, OmtpConstants.PROTOCOL_VERSION1_1, null);
                break;
            case TelephonyManager.VVM_TYPE_CVVM:
                messageSender = new OmtpCvvmMessageSender(smsManager, (short) applicationPort,
                        destinationNumber);
                break;
            default:
                Log.w(TAG, "Unexpected visual voicemail type: " + mVvmType);
        }

        return messageSender;
    }
}