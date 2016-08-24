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

package com.android.server.wifi.nan;

import static org.hamcrest.core.IsEqual.equalTo;
import static org.junit.Assert.assertEquals;

import android.net.wifi.nan.ConfigRequest;
import android.net.wifi.nan.PublishData;
import android.net.wifi.nan.PublishSettings;
import android.net.wifi.nan.SubscribeData;
import android.net.wifi.nan.SubscribeSettings;
import android.os.Parcel;
import android.test.suitebuilder.annotation.SmallTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ErrorCollector;
import org.junit.rules.ExpectedException;

/**
 * Unit test harness for WifiNanManager class.
 */
@SmallTest
public class WifiNanManagerTest {
    @Rule
    public ErrorCollector collector = new ErrorCollector();

    @Rule
    public ExpectedException thrown = ExpectedException.none();

    /*
     * ConfigRequest Tests
     */

    @Test
    public void testConfigRequestBuilder() {
        final int clusterHigh = 100;
        final int clusterLow = 5;
        final int masterPreference = 55;
        final boolean supportBand5g = true;

        ConfigRequest configRequest = new ConfigRequest.Builder().setClusterHigh(clusterHigh)
                .setClusterLow(clusterLow).setMasterPreference(masterPreference)
                .setSupport5gBand(supportBand5g).build();

        collector.checkThat("mClusterHigh", clusterHigh, equalTo(configRequest.mClusterHigh));
        collector.checkThat("mClusterLow", clusterLow, equalTo(configRequest.mClusterLow));
        collector.checkThat("mMasterPreference", masterPreference,
                equalTo(configRequest.mMasterPreference));
        collector.checkThat("mSupport5gBand", supportBand5g, equalTo(configRequest.mSupport5gBand));
    }

    @Test
    public void testConfigRequestBuilderMasterPrefNegative() {
        thrown.expect(IllegalArgumentException.class);
        ConfigRequest.Builder builder = new ConfigRequest.Builder();
        builder.setMasterPreference(-1);
    }

    @Test
    public void testConfigRequestBuilderMasterPrefReserved1() {
        thrown.expect(IllegalArgumentException.class);
        new ConfigRequest.Builder().setMasterPreference(1);
    }

    @Test
    public void testConfigRequestBuilderMasterPrefReserved255() {
        thrown.expect(IllegalArgumentException.class);
        new ConfigRequest.Builder().setMasterPreference(255);
    }

    @Test
    public void testConfigRequestBuilderMasterPrefTooLarge() {
        thrown.expect(IllegalArgumentException.class);
        new ConfigRequest.Builder().setMasterPreference(256);
    }

    @Test
    public void testConfigRequestBuilderClusterLowNegative() {
        thrown.expect(IllegalArgumentException.class);
        new ConfigRequest.Builder().setClusterLow(-1);
    }

    @Test
    public void testConfigRequestBuilderClusterHighNegative() {
        thrown.expect(IllegalArgumentException.class);
        new ConfigRequest.Builder().setClusterHigh(-1);
    }

    @Test
    public void testConfigRequestBuilderClusterLowAboveMax() {
        thrown.expect(IllegalArgumentException.class);
        new ConfigRequest.Builder().setClusterLow(ConfigRequest.CLUSTER_ID_MAX + 1);
    }

    @Test
    public void testConfigRequestBuilderClusterHighAboveMax() {
        thrown.expect(IllegalArgumentException.class);
        new ConfigRequest.Builder().setClusterHigh(ConfigRequest.CLUSTER_ID_MAX + 1);
    }

    @Test
    public void testConfigRequestBuilderClusterLowLargerThanHigh() {
        thrown.expect(IllegalArgumentException.class);
        ConfigRequest configRequest = new ConfigRequest.Builder().setClusterLow(100)
                .setClusterHigh(5).build();
    }

    @Test
    public void testConfigRequestParcel() {
        final int clusterHigh = 189;
        final int clusterLow = 25;
        final int masterPreference = 177;
        final boolean supportBand5g = true;

        ConfigRequest configRequest = new ConfigRequest.Builder().setClusterHigh(clusterHigh)
                .setClusterLow(clusterLow).setMasterPreference(masterPreference)
                .setSupport5gBand(supportBand5g).build();

        Parcel parcelW = Parcel.obtain();
        configRequest.writeToParcel(parcelW, 0);
        byte[] bytes = parcelW.marshall();
        parcelW.recycle();

        Parcel parcelR = Parcel.obtain();
        parcelR.unmarshall(bytes, 0, bytes.length);
        parcelR.setDataPosition(0);
        ConfigRequest rereadConfigRequest = ConfigRequest.CREATOR.createFromParcel(parcelR);

        assertEquals(configRequest, rereadConfigRequest);
    }

    /*
     * SubscribeData Tests
     */

    @Test
    public void testSubscribeDataBuilder() {
        final String serviceName = "some_service_or_other";
        final String serviceSpecificInfo = "long arbitrary string with some info";
        final byte[] txFilter = {
                0, 1, 16, 1, 22 };
        final byte[] rxFilter = {
                1, 127, 0, 1, -5, 1, 22 };

        SubscribeData subscribeData = new SubscribeData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(serviceSpecificInfo).setTxFilter(txFilter, txFilter.length)
                .setRxFilter(rxFilter, rxFilter.length).build();

        collector.checkThat("mServiceName", serviceName, equalTo(subscribeData.mServiceName));
        String mServiceSpecificInfo = new String(subscribeData.mServiceSpecificInfo, 0,
                subscribeData.mServiceSpecificInfoLength);
        collector.checkThat("mServiceSpecificInfo",
                utilAreArraysEqual(serviceSpecificInfo.getBytes(), serviceSpecificInfo.length(),
                        subscribeData.mServiceSpecificInfo,
                        subscribeData.mServiceSpecificInfoLength),
                equalTo(true));
        collector.checkThat("mTxFilter", utilAreArraysEqual(txFilter, txFilter.length,
                subscribeData.mTxFilter, subscribeData.mTxFilterLength), equalTo(true));
        collector.checkThat("mRxFilter", utilAreArraysEqual(rxFilter, rxFilter.length,
                subscribeData.mRxFilter, subscribeData.mRxFilterLength), equalTo(true));
    }

    @Test
    public void testSubscribeDataParcel() {
        final String serviceName = "some_service_or_other";
        final String serviceSpecificInfo = "long arbitrary string with some info";
        final byte[] txFilter = {
                0, 1, 16, 1, 22 };
        final byte[] rxFilter = {
                1, 127, 0, 1, -5, 1, 22 };

        SubscribeData subscribeData = new SubscribeData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(serviceSpecificInfo).setTxFilter(txFilter, txFilter.length)
                .setTxFilter(rxFilter, rxFilter.length).build();

        Parcel parcelW = Parcel.obtain();
        subscribeData.writeToParcel(parcelW, 0);
        byte[] bytes = parcelW.marshall();
        parcelW.recycle();

        Parcel parcelR = Parcel.obtain();
        parcelR.unmarshall(bytes, 0, bytes.length);
        parcelR.setDataPosition(0);
        SubscribeData rereadSubscribeData = SubscribeData.CREATOR.createFromParcel(parcelR);

        assertEquals(subscribeData, rereadSubscribeData);
    }

    /*
     * SubscribeSettings Tests
     */

    @Test
    public void testSubscribeSettingsBuilder() {
        final int subscribeType = SubscribeSettings.SUBSCRIBE_TYPE_PASSIVE;
        final int subscribeCount = 10;
        final int subscribeTtl = 15;

        SubscribeSettings subscribeSetting = new SubscribeSettings.Builder()
                .setSubscribeType(subscribeType).setSubscribeCount(subscribeCount)
                .setTtlSec(subscribeTtl).build();

        collector.checkThat("mSubscribeType", subscribeType,
                equalTo(subscribeSetting.mSubscribeType));
        collector.checkThat("mSubscribeCount", subscribeCount,
                equalTo(subscribeSetting.mSubscribeCount));
        collector.checkThat("mTtlSec", subscribeTtl, equalTo(subscribeSetting.mTtlSec));
    }

    @Test
    public void testSubscribeSettingsBuilderBadSubscribeType() {
        thrown.expect(IllegalArgumentException.class);
        new SubscribeSettings.Builder().setSubscribeType(10);
    }

    @Test
    public void testSubscribeSettingsBuilderNegativeCount() {
        thrown.expect(IllegalArgumentException.class);
        new SubscribeSettings.Builder().setSubscribeCount(-1);
    }

    @Test
    public void testSubscribeSettingsBuilderNegativeTtl() {
        thrown.expect(IllegalArgumentException.class);
        new SubscribeSettings.Builder().setTtlSec(-100);
    }

    @Test
    public void testSubscribeSettingsParcel() {
        final int subscribeType = SubscribeSettings.SUBSCRIBE_TYPE_PASSIVE;
        final int subscribeCount = 10;
        final int subscribeTtl = 15;

        SubscribeSettings subscribeSetting = new SubscribeSettings.Builder()
                .setSubscribeType(subscribeType).setSubscribeCount(subscribeCount)
                .setTtlSec(subscribeTtl).build();

        Parcel parcelW = Parcel.obtain();
        subscribeSetting.writeToParcel(parcelW, 0);
        byte[] bytes = parcelW.marshall();
        parcelW.recycle();

        Parcel parcelR = Parcel.obtain();
        parcelR.unmarshall(bytes, 0, bytes.length);
        parcelR.setDataPosition(0);
        SubscribeSettings rereadSubscribeSettings = SubscribeSettings.CREATOR
                .createFromParcel(parcelR);

        assertEquals(subscribeSetting, rereadSubscribeSettings);
    }

    /*
     * PublishData Tests
     */

    @Test
    public void testPublishDataBuilder() {
        final String serviceName = "some_service_or_other";
        final String serviceSpecificInfo = "long arbitrary string with some info";
        final byte[] txFilter = {
                0, 1, 16, 1, 22 };
        final byte[] rxFilter = {
                1, 127, 0, 1, -5, 1, 22 };

        PublishData publishData = new PublishData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(serviceSpecificInfo).setTxFilter(txFilter, txFilter.length)
                .setRxFilter(rxFilter, rxFilter.length).build();

        collector.checkThat("mServiceName", serviceName, equalTo(publishData.mServiceName));
        String mServiceSpecificInfo = new String(publishData.mServiceSpecificInfo, 0,
                publishData.mServiceSpecificInfoLength);
        collector.checkThat("mServiceSpecificInfo",
                utilAreArraysEqual(serviceSpecificInfo.getBytes(), serviceSpecificInfo.length(),
                        publishData.mServiceSpecificInfo, publishData.mServiceSpecificInfoLength),
                equalTo(true));
        collector.checkThat("mTxFilter", utilAreArraysEqual(txFilter, txFilter.length,
                publishData.mTxFilter, publishData.mTxFilterLength), equalTo(true));
        collector.checkThat("mRxFilter", utilAreArraysEqual(rxFilter, rxFilter.length,
                publishData.mRxFilter, publishData.mRxFilterLength), equalTo(true));
    }

    @Test
    public void testPublishDataParcel() {
        final String serviceName = "some_service_or_other";
        final String serviceSpecificInfo = "long arbitrary string with some info";
        final byte[] txFilter = {
                0, 1, 16, 1, 22 };
        final byte[] rxFilter = {
                1, 127, 0, 1, -5, 1, 22 };

        PublishData publishData = new PublishData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(serviceSpecificInfo).setTxFilter(txFilter, txFilter.length)
                .setTxFilter(rxFilter, rxFilter.length).build();

        Parcel parcelW = Parcel.obtain();
        publishData.writeToParcel(parcelW, 0);
        byte[] bytes = parcelW.marshall();
        parcelW.recycle();

        Parcel parcelR = Parcel.obtain();
        parcelR.unmarshall(bytes, 0, bytes.length);
        parcelR.setDataPosition(0);
        PublishData rereadPublishData = PublishData.CREATOR.createFromParcel(parcelR);

        assertEquals(publishData, rereadPublishData);
    }

    /*
     * PublishSettings Tests
     */

    @Test
    public void testPublishSettingsBuilder() {
        final int publishType = PublishSettings.PUBLISH_TYPE_SOLICITED;
        final int publishCount = 10;
        final int publishTtl = 15;

        PublishSettings publishSetting = new PublishSettings.Builder().setPublishType(publishType)
                .setPublishCount(publishCount).setTtlSec(publishTtl).build();

        collector.checkThat("mPublishType", publishType, equalTo(publishSetting.mPublishType));
        collector.checkThat("mPublishCount", publishCount, equalTo(publishSetting.mPublishCount));
        collector.checkThat("mTtlSec", publishTtl, equalTo(publishSetting.mTtlSec));
    }

    @Test
    public void testPublishSettingsBuilderBadPublishType() {
        thrown.expect(IllegalArgumentException.class);
        new PublishSettings.Builder().setPublishType(5);
    }

    @Test
    public void testPublishSettingsBuilderNegativeCount() {
        thrown.expect(IllegalArgumentException.class);
        new PublishSettings.Builder().setPublishCount(-4);
    }

    @Test
    public void testPublishSettingsBuilderNegativeTtl() {
        thrown.expect(IllegalArgumentException.class);
        new PublishSettings.Builder().setTtlSec(-10);
    }

    @Test
    public void testPublishSettingsParcel() {
        final int publishType = PublishSettings.PUBLISH_TYPE_SOLICITED;
        final int publishCount = 10;
        final int publishTtl = 15;

        PublishSettings configSetting = new PublishSettings.Builder().setPublishType(publishType)
                .setPublishCount(publishCount).setTtlSec(publishTtl).build();

        Parcel parcelW = Parcel.obtain();
        configSetting.writeToParcel(parcelW, 0);
        byte[] bytes = parcelW.marshall();
        parcelW.recycle();

        Parcel parcelR = Parcel.obtain();
        parcelR.unmarshall(bytes, 0, bytes.length);
        parcelR.setDataPosition(0);
        PublishSettings rereadPublishSettings = PublishSettings.CREATOR.createFromParcel(parcelR);

        assertEquals(configSetting, rereadPublishSettings);
    }

    /*
     * Utilities
     */

    private static boolean utilAreArraysEqual(byte[] x, int xLength, byte[] y, int yLength) {
        if (xLength != yLength) {
            return false;
        }

        if (x != null && y != null) {
            for (int i = 0; i < xLength; ++i) {
                if (x[i] != y[i]) {
                    return false;
                }
            }
        } else if (xLength != 0) {
            return false; // invalid != invalid
        }

        return true;
    }
}
