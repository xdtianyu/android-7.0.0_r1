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
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyZeroInteractions;

import android.net.wifi.nan.ConfigRequest;
import android.net.wifi.nan.PublishData;
import android.net.wifi.nan.PublishSettings;
import android.net.wifi.nan.SubscribeData;
import android.net.wifi.nan.SubscribeSettings;
import android.net.wifi.nan.TlvBufferUtils;
import android.net.wifi.nan.WifiNanSessionListener;
import android.os.Bundle;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.server.wifi.HalMockUtils;
import com.android.server.wifi.WifiNative;

import libcore.util.HexEncoding;

import org.json.JSONException;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ErrorCollector;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.lang.reflect.Field;
import java.util.Arrays;

/**
 * Unit test harness for WifiNanNative + JNI code interfacing to the HAL.
 */
@SmallTest
public class WifiNanHalTest {
    private WifiNanNative mDut = WifiNanNative.getInstance();
    private ArgumentCaptor<String> mArgs = ArgumentCaptor.forClass(String.class);

    @Mock
    private WifiNanHalMock mNanHalMock;
    @Mock private WifiNanStateManager mNanStateManager;

    @Rule
    public ErrorCollector collector = new ErrorCollector();

    @Before
    public void setup() throws Exception {
        MockitoAnnotations.initMocks(this);

        HalMockUtils.initHalMockLibrary();
        WifiNanHalMock.initNanHalMockLibrary();
        WifiNanNative.initNanHandlersNative(WifiNative.class, WifiNative.sWlan0Index);
        HalMockUtils.setHalMockObject(mNanHalMock);
        installMockNanStateManager(mNanStateManager);
    }

    @Test
    public void testEnableWith5g() throws JSONException {
        final short transactionId = 2346;
        final int clusterLow = 23;
        final int clusterHigh = 126;
        final int masterPref = 234;
        final boolean enable5g = true;

        testEnable(transactionId, clusterLow, clusterHigh, masterPref, enable5g);
    }

    @Test
    public void testEnableWithout5g() throws JSONException {
        final short transactionId = 1296;
        final int clusterLow = 17;
        final int clusterHigh = 197;
        final int masterPref = 33;
        final boolean enable5g = false;

        testEnable(transactionId, clusterLow, clusterHigh, masterPref, enable5g);
    }

    @Test
    public void testDisable() {
        final short transactionId = 5478;

        mDut.disable(transactionId);

        verify(mNanHalMock).disableHalMockNative(transactionId);
    }

    @Test
    public void testPublishUnsolicited() throws JSONException {
        final short transactionId = 55;
        final int publishId = 23;
        final String serviceName = "some-service-name";
        final String ssi = "some much longer and more arbitrary data";
        final int publishCount = 7;
        final int publishTtl = 66;

        TlvBufferUtils.TlvConstructor tlvTx = new TlvBufferUtils.TlvConstructor(0, 1);
        tlvTx.allocate(150).putByte(0, (byte) 10).putInt(0, 100).putString(0, "some string")
                .putZeroLengthElement(0);

        TlvBufferUtils.TlvConstructor tlvRx = new TlvBufferUtils.TlvConstructor(0, 1);
        tlvRx.allocate(150).putByte(0, (byte) 66).putInt(0, 127).putString(0, "some other string")
                .putZeroLengthElement(0).putByteArray(0, serviceName.getBytes());

        testPublish(transactionId, publishId, PublishSettings.PUBLISH_TYPE_UNSOLICITED, serviceName,
                ssi, tlvTx, tlvRx, publishCount, publishTtl);
    }

    @Test
    public void testPublishSolicited() throws JSONException {
        final short transactionId = 45;
        final int publishId = 17;
        final String serviceName = "some-service-name-or-another";
        final String ssi = "some much longer arbitrary data";
        final int publishCount = 32;
        final int publishTtl = 33;

        TlvBufferUtils.TlvConstructor tlvTx = new TlvBufferUtils.TlvConstructor(0, 1);
        tlvTx.allocate(150).putByte(0, (byte) 10).putInt(0, 100).putString(0, "some string")
                .putZeroLengthElement(0);

        TlvBufferUtils.TlvConstructor tlvRx = new TlvBufferUtils.TlvConstructor(0, 1);
        tlvRx.allocate(150).putByte(0, (byte) 66).putInt(0, 127).putString(0, "some other string")
                .putZeroLengthElement(0).putByteArray(0, serviceName.getBytes());

        testPublish(transactionId, publishId, PublishSettings.PUBLISH_TYPE_SOLICITED, serviceName,
                ssi, tlvTx, tlvRx, publishCount, publishTtl);
    }

    @Test
    public void testPublishCancel() throws JSONException {
        final short transactionId = 12;
        final int publishId = 15;

        mDut.stopPublish(transactionId, publishId);

        verify(mNanHalMock).publishCancelHalMockNative(eq(transactionId), mArgs.capture());

        Bundle argsData = HalMockUtils.convertJsonToBundle(mArgs.getValue());

        collector.checkThat("publish_id", argsData.getInt("publish_id"), equalTo(publishId));
    }

    @Test
    public void testSubscribePassive() throws JSONException {
        final short transactionId = 45;
        final int subscribeId = 17;
        final String serviceName = "some-service-name-or-another";
        final String ssi = "some much longer arbitrary data";
        final int subscribeCount = 32;
        final int subscribeTtl = 33;

        TlvBufferUtils.TlvConstructor tlvTx = new TlvBufferUtils.TlvConstructor(0, 1);
        tlvTx.allocate(150).putByte(0, (byte) 10).putInt(0, 100).putString(0, "some string")
                .putZeroLengthElement(0);

        TlvBufferUtils.TlvConstructor tlvRx = new TlvBufferUtils.TlvConstructor(0, 1);
        tlvRx.allocate(150).putByte(0, (byte) 66).putInt(0, 127).putString(0, "some other string")
                .putZeroLengthElement(0).putByteArray(0, serviceName.getBytes());

        testSubscribe(transactionId, subscribeId, SubscribeSettings.SUBSCRIBE_TYPE_PASSIVE,
                serviceName, ssi, tlvTx, tlvRx, subscribeCount, subscribeTtl);
    }

    @Test
    public void testSubscribeActive() throws JSONException {
        final short transactionId = 45;
        final int subscribeId = 17;
        final String serviceName = "some-service-name-or-another";
        final String ssi = "some much longer arbitrary data";
        final int subscribeCount = 32;
        final int subscribeTtl = 33;

        TlvBufferUtils.TlvConstructor tlvTx = new TlvBufferUtils.TlvConstructor(0, 1);
        tlvTx.allocate(150).putByte(0, (byte) 10).putInt(0, 100).putString(0, "some string")
                .putZeroLengthElement(0);

        TlvBufferUtils.TlvConstructor tlvRx = new TlvBufferUtils.TlvConstructor(0, 1);
        tlvRx.allocate(150).putByte(0, (byte) 66).putInt(0, 127).putString(0, "some other string")
                .putZeroLengthElement(0).putByteArray(0, serviceName.getBytes());

        testSubscribe(transactionId, subscribeId, SubscribeSettings.SUBSCRIBE_TYPE_ACTIVE,
                serviceName, ssi, tlvTx, tlvRx, subscribeCount, subscribeTtl);
    }

    @Test
    public void testSubscribeCancel() throws JSONException {
        final short transactionId = 12;
        final int subscribeId = 15;

        mDut.stopSubscribe(transactionId, subscribeId);

        verify(mNanHalMock).subscribeCancelHalMockNative(eq(transactionId), mArgs.capture());

        Bundle argsData = HalMockUtils.convertJsonToBundle(mArgs.getValue());

        collector.checkThat("subscribe_id", argsData.getInt("subscribe_id"), equalTo(subscribeId));
    }

    @Test
    public void testSendMessage() throws JSONException {
        final short transactionId = 45;
        final int pubSubId = 22;
        final int reqInstanceId = 11;
        final byte[] peer = HexEncoding.decode("000102030405".toCharArray(), false);
        final String msg = "Hello there - how are you doing?";

        mDut.sendMessage(transactionId, pubSubId, reqInstanceId, peer, msg.getBytes(),
                msg.length());

        verify(mNanHalMock).transmitFollowupHalMockNative(eq(transactionId), mArgs.capture());

        Bundle argsData = HalMockUtils.convertJsonToBundle(mArgs.getValue());

        collector.checkThat("publish_subscribe_id", argsData.getInt("publish_subscribe_id"),
                equalTo(pubSubId));
        collector.checkThat("requestor_instance_id", argsData.getInt("requestor_instance_id"),
                equalTo(reqInstanceId));
        collector.checkThat("addr", argsData.getByteArray("addr"), equalTo(peer));
        collector.checkThat("priority", argsData.getInt("priority"), equalTo(0));
        collector.checkThat("dw_or_faw", argsData.getInt("dw_or_faw"), equalTo(0));
        collector.checkThat("service_specific_info_len",
                argsData.getInt("service_specific_info_len"), equalTo(msg.length()));
        collector.checkThat("service_specific_info", argsData.getByteArray("service_specific_info"),
                equalTo(msg.getBytes()));
    }

    @Test
    public void testNotifyCapabilities() throws JSONException {
        final short transactionId = 23;
        final int max_concurrent_nan_clusters = 1;
        final int max_publishes = 2;
        final int max_subscribes = 3;
        final int max_service_name_len = 4;
        final int max_match_filter_len = 5;
        final int max_total_match_filter_len = 6;
        final int max_service_specific_info_len = 7;
        final int max_vsa_data_len = 8;
        final int max_mesh_data_len = 9;
        final int max_ndi_interfaces = 10;
        final int max_ndp_sessions = 11;
        final int max_app_info_len = 12;

        ArgumentCaptor<WifiNanNative.Capabilities> capabilitiesCapture = ArgumentCaptor
                .forClass(WifiNanNative.Capabilities.class);

        Bundle args = new Bundle();
        args.putInt("status", WifiNanNative.NAN_STATUS_SUCCESS);
        args.putInt("value", 0);
        args.putInt("response_type", WifiNanNative.NAN_RESPONSE_GET_CAPABILITIES);

        args.putInt("body.nan_capabilities.max_concurrent_nan_clusters",
                max_concurrent_nan_clusters);
        args.putInt("body.nan_capabilities.max_publishes", max_publishes);
        args.putInt("body.nan_capabilities.max_subscribes", max_subscribes);
        args.putInt("body.nan_capabilities.max_service_name_len", max_service_name_len);
        args.putInt("body.nan_capabilities.max_match_filter_len", max_match_filter_len);
        args.putInt("body.nan_capabilities.max_total_match_filter_len", max_total_match_filter_len);
        args.putInt("body.nan_capabilities.max_service_specific_info_len",
                max_service_specific_info_len);
        args.putInt("body.nan_capabilities.max_vsa_data_len", max_vsa_data_len);
        args.putInt("body.nan_capabilities.max_mesh_data_len", max_mesh_data_len);
        args.putInt("body.nan_capabilities.max_ndi_interfaces", max_ndi_interfaces);
        args.putInt("body.nan_capabilities.max_ndp_sessions", max_ndp_sessions);
        args.putInt("body.nan_capabilities.max_app_info_len", max_app_info_len);

        WifiNanHalMock.callNotifyResponse(transactionId,
                HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onCapabilitiesUpdate(eq(transactionId),
                capabilitiesCapture.capture());
        WifiNanNative.Capabilities capabilities = capabilitiesCapture.getValue();
        collector.checkThat("max_concurrent_nan_clusters", capabilities.maxConcurrentNanClusters,
                equalTo(max_concurrent_nan_clusters));
        collector.checkThat("max_publishes", capabilities.maxPublishes, equalTo(max_publishes));
        collector.checkThat("max_subscribes", capabilities.maxSubscribes, equalTo(max_subscribes));
        collector.checkThat("max_service_name_len", capabilities.maxServiceNameLen,
                equalTo(max_service_name_len));
        collector.checkThat("max_match_filter_len", capabilities.maxMatchFilterLen,
                equalTo(max_match_filter_len));
        collector.checkThat("max_total_match_filter_len", capabilities.maxTotalMatchFilterLen,
                equalTo(max_total_match_filter_len));
        collector.checkThat("max_service_specific_info_len", capabilities.maxServiceSpecificInfoLen,
                equalTo(max_service_specific_info_len));
        collector.checkThat("max_vsa_data_len", capabilities.maxVsaDataLen,
                equalTo(max_vsa_data_len));
        collector.checkThat("max_mesh_data_len", capabilities.maxMeshDataLen,
                equalTo(max_mesh_data_len));
        collector.checkThat("max_ndi_interfaces", capabilities.maxNdiInterfaces,
                equalTo(max_ndi_interfaces));
        collector.checkThat("max_ndp_sessions", capabilities.maxNdpSessions,
                equalTo(max_ndp_sessions));
        collector.checkThat("max_app_info_len", capabilities.maxAppInfoLen,
                equalTo(max_app_info_len));
    }

    @Test
    public void testNotifyResponseConfigSuccess() throws JSONException {
        final short transactionId = 23;

        Bundle args = new Bundle();
        args.putInt("status", WifiNanNative.NAN_STATUS_SUCCESS);
        args.putInt("value", 0);
        args.putInt("response_type", WifiNanNative.NAN_RESPONSE_ENABLED);

        WifiNanHalMock.callNotifyResponse(transactionId,
                HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onConfigCompleted(transactionId);
    }

    @Test
    public void testNotifyResponseConfigFail() throws JSONException {
        final short transactionId = 23;

        Bundle args = new Bundle();
        args.putInt("status", WifiNanNative.NAN_STATUS_INVALID_BAND_CONFIG_FLAGS);
        args.putInt("value", 0);
        args.putInt("response_type", WifiNanNative.NAN_RESPONSE_ENABLED);

        WifiNanHalMock.callNotifyResponse(transactionId,
                HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onConfigFailed(transactionId,
                WifiNanSessionListener.FAIL_REASON_INVALID_ARGS);
    }

    @Test
    public void testNotifyResponsePublishSuccess() throws JSONException {
        final short transactionId = 23;
        final int publishId = 127;

        Bundle args = new Bundle();
        args.putInt("status", WifiNanNative.NAN_STATUS_SUCCESS);
        args.putInt("value", 0);
        args.putInt("response_type", WifiNanNative.NAN_RESPONSE_PUBLISH);
        args.putInt("body.publish_response.publish_id", publishId);

        WifiNanHalMock.callNotifyResponse(transactionId,
                HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onPublishSuccess(transactionId, publishId);
    }

    @Test
    public void testNotifyResponsePublishFail() throws JSONException {
        final short transactionId = 23;
        final int publishId = 127;

        Bundle args = new Bundle();
        args.putInt("status", WifiNanNative.NAN_STATUS_NO_SPACE_AVAILABLE);
        args.putInt("value", 57);
        args.putInt("response_type", WifiNanNative.NAN_RESPONSE_PUBLISH);
        args.putInt("body.publish_response.publish_id", publishId);

        WifiNanHalMock.callNotifyResponse(transactionId,
                HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onPublishFail(transactionId,
                WifiNanSessionListener.FAIL_REASON_NO_RESOURCES);
    }

    @Test
    public void testNotifyResponsePublishCancel() throws JSONException {
        final short transactionId = 23;
        final int publishId = 127;

        Bundle args = new Bundle();
        args.putInt("status", WifiNanNative.NAN_STATUS_SUCCESS);
        args.putInt("value", 0);
        args.putInt("response_type", WifiNanNative.NAN_RESPONSE_PUBLISH_CANCEL);

        WifiNanHalMock.callNotifyResponse(transactionId,
                HalMockUtils.convertBundleToJson(args).toString());

        verifyZeroInteractions(mNanStateManager);
    }

    @Test
    public void testNotifyResponseSubscribeSuccess() throws JSONException {
        final short transactionId = 17;
        final int subscribeId = 198;

        Bundle args = new Bundle();
        args.putInt("status", WifiNanNative.NAN_STATUS_SUCCESS);
        args.putInt("value", 0);
        args.putInt("response_type", WifiNanNative.NAN_RESPONSE_SUBSCRIBE);
        args.putInt("body.subscribe_response.subscribe_id", subscribeId);

        WifiNanHalMock.callNotifyResponse(transactionId,
                HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onSubscribeSuccess(transactionId, subscribeId);
    }

    @Test
    public void testNotifyResponseSubscribeFail() throws JSONException {
        final short transactionId = 17;
        final int subscribeId = 198;

        Bundle args = new Bundle();
        args.putInt("status", WifiNanNative.NAN_STATUS_DE_FAILURE);
        args.putInt("value", 0);
        args.putInt("response_type", WifiNanNative.NAN_RESPONSE_SUBSCRIBE);
        args.putInt("body.subscribe_response.subscribe_id", subscribeId);

        WifiNanHalMock.callNotifyResponse(transactionId,
                HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onSubscribeFail(transactionId,
                WifiNanSessionListener.FAIL_REASON_OTHER);
    }

    @Test
    public void testNotifyResponseSubscribeCancel() throws JSONException {
        final short transactionId = 23;
        final int subscribeId = 127;

        Bundle args = new Bundle();
        args.putInt("status", WifiNanNative.NAN_STATUS_SUCCESS);
        args.putInt("value", 0);
        args.putInt("response_type", WifiNanNative.NAN_RESPONSE_SUBSCRIBE_CANCEL);

        WifiNanHalMock.callNotifyResponse(transactionId,
                HalMockUtils.convertBundleToJson(args).toString());

        verifyZeroInteractions(mNanStateManager);
    }

    @Test
    public void testNotifyResponseTransmitFollowupSuccess() throws JSONException {
        final short transactionId = 23;

        Bundle args = new Bundle();
        args.putInt("status", WifiNanNative.NAN_STATUS_SUCCESS);
        args.putInt("value", 0);
        args.putInt("response_type", WifiNanNative.NAN_RESPONSE_TRANSMIT_FOLLOWUP);

        WifiNanHalMock.callNotifyResponse(transactionId,
                HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onMessageSendSuccess(transactionId);
    }

    @Test
    public void testNotifyResponseTransmitFollowupFail() throws JSONException {
        final short transactionId = 45;

        Bundle args = new Bundle();
        args.putInt("status", WifiNanNative.NAN_STATUS_TIMEOUT);
        args.putInt("value", 0);
        args.putInt("response_type", WifiNanNative.NAN_RESPONSE_TRANSMIT_FOLLOWUP);

        WifiNanHalMock.callNotifyResponse(transactionId,
                HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onMessageSendFail(transactionId,
                WifiNanSessionListener.FAIL_REASON_OTHER);
    }

    @Test
    public void testPublishTerminatedDone() throws JSONException {
        final int publishId = 167;

        Bundle args = new Bundle();
        args.putInt("publish_id", publishId);
        args.putInt("reason", WifiNanNative.NAN_TERMINATED_REASON_COUNT_REACHED);

        WifiNanHalMock.callPublishTerminated(HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onPublishTerminated(publishId,
                WifiNanSessionListener.TERMINATE_REASON_DONE);
    }

    @Test
    public void testSubscribeTerminatedFail() throws JSONException {
        final int subscribeId = 167;

        Bundle args = new Bundle();
        args.putInt("subscribe_id", subscribeId);
        args.putInt("reason", WifiNanNative.NAN_TERMINATED_REASON_FAILURE);

        WifiNanHalMock.callSubscribeTerminated(HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onSubscribeTerminated(subscribeId,
                WifiNanSessionListener.TERMINATE_REASON_FAIL);
    }

    @Test
    public void testFollowup() throws JSONException {
        final int pubSubId = 236;
        final int reqInstanceId = 57;
        final byte[] peer = HexEncoding.decode("0A0B0C0D0E0F".toCharArray(), false);
        final String message = "this is some message received from some peer - hello!";

        Bundle args = new Bundle();
        args.putInt("publish_subscribe_id", pubSubId);
        args.putInt("requestor_instance_id", reqInstanceId);
        args.putByteArray("addr", peer);
        args.putInt("dw_or_faw", 0);
        args.putInt("service_specific_info_len", message.length());
        args.putByteArray("service_specific_info", message.getBytes());

        WifiNanHalMock.callFollowup(HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onMessageReceived(pubSubId, reqInstanceId, peer,
                message.getBytes(), message.length());
    }

    @Test
    public void testMatch() throws JSONException {
        final int pubSubId = 287;
        final int reqInstanceId = 98;
        final byte[] peer = HexEncoding.decode("010203040506".toCharArray(), false);
        final String ssi = "some service specific info - really arbitrary";
        final String filter = "most likely binary - but faking here with some string data";

        Bundle args = new Bundle();
        args.putInt("publish_subscribe_id", pubSubId);
        args.putInt("requestor_instance_id", reqInstanceId);
        args.putByteArray("addr", peer);
        args.putInt("service_specific_info_len", ssi.length());
        args.putByteArray("service_specific_info", ssi.getBytes());
        args.putInt("sdf_match_filter_len", filter.length());
        args.putByteArray("sdf_match_filter", filter.getBytes());

        WifiNanHalMock.callMatch(HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onMatch(pubSubId, reqInstanceId, peer, ssi.getBytes(),
                ssi.length(), filter.getBytes(), filter.length());
    }

    @Test
    public void testDiscoveryInterfaceChange() throws JSONException {
        final byte[] mac = HexEncoding.decode("060504030201".toCharArray(), false);

        Bundle args = new Bundle();
        args.putInt("event_type", WifiNanNative.NAN_EVENT_ID_DISC_MAC_ADDR);
        args.putByteArray("data", mac);

        WifiNanHalMock.callDiscEngEvent(HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onInterfaceAddressChange(mac);
    }

    @Test
    public void testClusterChange() throws JSONException {
        final byte[] mac = HexEncoding.decode("060504030201".toCharArray(), false);

        Bundle args = new Bundle();
        args.putInt("event_type", WifiNanNative.NAN_EVENT_ID_JOINED_CLUSTER);
        args.putByteArray("data", mac);

        WifiNanHalMock.callDiscEngEvent(HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onClusterChange(WifiNanClientState.CLUSTER_CHANGE_EVENT_JOINED,
                mac);
    }

    @Test
    public void testDisabled() throws JSONException {
        Bundle args = new Bundle();
        args.putInt("reason", WifiNanNative.NAN_STATUS_DE_FAILURE);

        WifiNanHalMock.callDisabled(HalMockUtils.convertBundleToJson(args).toString());

        verify(mNanStateManager).onNanDown(WifiNanSessionListener.FAIL_REASON_OTHER);
    }

    /*
     * Utilities
     */

    private void testEnable(short transactionId, int clusterLow, int clusterHigh, int masterPref,
            boolean enable5g) throws JSONException {
        ConfigRequest configRequest = new ConfigRequest.Builder().setClusterLow(clusterLow)
                .setClusterHigh(clusterHigh).setMasterPreference(masterPref)
                .setSupport5gBand(enable5g).build();

        mDut.enableAndConfigure(transactionId, configRequest);

        verify(mNanHalMock).enableHalMockNative(eq(transactionId), mArgs.capture());

        Bundle argsData = HalMockUtils.convertJsonToBundle(mArgs.getValue());

        collector.checkThat("master_pref", argsData.getInt("master_pref"), equalTo(masterPref));
        collector.checkThat("cluster_low", argsData.getInt("cluster_low"), equalTo(clusterLow));
        collector.checkThat("cluster_high", argsData.getInt("cluster_high"), equalTo(clusterHigh));
        collector.checkThat("config_support_5g", argsData.getInt("config_support_5g"), equalTo(1));
        collector.checkThat("support_5g_val", argsData.getInt("support_5g_val"),
                equalTo(enable5g ? 1 : 0));

        collector.checkThat("config_sid_beacon", argsData.getInt("config_sid_beacon"), equalTo(0));
        collector.checkThat("config_2dot4g_rssi_close", argsData.getInt("config_2dot4g_rssi_close"),
                equalTo(0));
        collector.checkThat("config_2dot4g_rssi_middle",
                argsData.getInt("config_2dot4g_rssi_middle"), equalTo(0));
        collector.checkThat("config_2dot4g_rssi_proximity",
                argsData.getInt("config_2dot4g_rssi_proximity"), equalTo(0));
        collector.checkThat("config_hop_count_limit", argsData.getInt("config_hop_count_limit"),
                equalTo(0));
        collector.checkThat("config_2dot4g_support", argsData.getInt("config_2dot4g_support"),
                equalTo(0));
        collector.checkThat("config_2dot4g_beacons", argsData.getInt("config_2dot4g_beacons"),
                equalTo(0));
        collector.checkThat("config_2dot4g_sdf", argsData.getInt("config_2dot4g_sdf"), equalTo(0));
        collector.checkThat("config_5g_beacons", argsData.getInt("config_5g_beacons"), equalTo(0));
        collector.checkThat("config_5g_sdf", argsData.getInt("config_5g_sdf"), equalTo(0));
        collector.checkThat("config_5g_rssi_close", argsData.getInt("config_5g_rssi_close"),
                equalTo(0));
        collector.checkThat("config_5g_rssi_middle", argsData.getInt("config_5g_rssi_middle"),
                equalTo(0));
        collector.checkThat("config_5g_rssi_close_proximity",
                argsData.getInt("config_5g_rssi_close_proximity"), equalTo(0));
        collector.checkThat("config_rssi_window_size", argsData.getInt("config_rssi_window_size"),
                equalTo(0));
        collector.checkThat("config_oui", argsData.getInt("config_oui"), equalTo(0));
        collector.checkThat("config_intf_addr", argsData.getInt("config_intf_addr"), equalTo(0));
        collector.checkThat("config_cluster_attribute_val",
                argsData.getInt("config_cluster_attribute_val"), equalTo(0));
        collector.checkThat("config_scan_params", argsData.getInt("config_scan_params"),
                equalTo(0));
        collector.checkThat("config_random_factor_force",
                argsData.getInt("config_random_factor_force"), equalTo(0));
        collector.checkThat("config_hop_count_force", argsData.getInt("config_hop_count_force"),
                equalTo(0));
    }

    private void testPublish(short transactionId, int publishId, int publishType,
            String serviceName, String ssi, TlvBufferUtils.TlvConstructor tlvTx,
            TlvBufferUtils.TlvConstructor tlvRx, int publishCount, int publishTtl)
                    throws JSONException {
        PublishData publishData = new PublishData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(ssi)
                .setTxFilter(tlvTx.getArray(), tlvTx.getActualLength())
                .setRxFilter(tlvRx.getArray(), tlvRx.getActualLength()).build();

        PublishSettings publishSettings = new PublishSettings.Builder().setPublishType(publishType)
                .setPublishCount(publishCount).setTtlSec(publishTtl).build();

        mDut.publish(transactionId, publishId, publishData, publishSettings);

        verify(mNanHalMock).publishHalMockNative(eq(transactionId), mArgs.capture());

        Bundle argsData = HalMockUtils.convertJsonToBundle(mArgs.getValue());

        collector.checkThat("publish_id", argsData.getInt("publish_id"), equalTo(publishId));
        collector.checkThat("ttl", argsData.getInt("ttl"), equalTo(publishTtl));
        collector.checkThat("publish_type", argsData.getInt("publish_type"), equalTo(publishType));
        collector.checkThat("tx_type", argsData.getInt("tx_type"),
                equalTo(publishType == PublishSettings.PUBLISH_TYPE_UNSOLICITED ? 0 : 1));
        collector.checkThat("publish_count", argsData.getInt("publish_count"),
                equalTo(publishCount));
        collector.checkThat("service_name_len", argsData.getInt("service_name_len"),
                equalTo(serviceName.length()));
        collector.checkThat("service_name", argsData.getByteArray("service_name"),
                equalTo(serviceName.getBytes()));
        collector.checkThat("service_specific_info_len",
                argsData.getInt("service_specific_info_len"), equalTo(ssi.length()));
        collector.checkThat("service_specific_info", argsData.getByteArray("service_specific_info"),
                equalTo(ssi.getBytes()));
        collector.checkThat("publish_match_indicator", argsData.getInt("publish_match_indicator"),
                equalTo(0));
        collector.checkThat("rx_match_filter_len", argsData.getInt("rx_match_filter_len"),
                equalTo(tlvRx.getActualLength()));
        collector.checkThat("rx_match_filter", argsData.getByteArray("rx_match_filter"),
                equalTo(Arrays.copyOf(tlvRx.getArray(), tlvRx.getActualLength())));
        collector.checkThat("tx_match_filter_len", argsData.getInt("tx_match_filter_len"),
                equalTo(tlvTx.getActualLength()));
        collector.checkThat("tx_match_filter", argsData.getByteArray("tx_match_filter"),
                equalTo(Arrays.copyOf(tlvTx.getArray(), tlvTx.getActualLength())));
        collector.checkThat("rssi_threshold_flag", argsData.getInt("rssi_threshold_flag"),
                equalTo(0));
        collector.checkThat("connmap", argsData.getInt("connmap"), equalTo(0));
    }

    private void testSubscribe(short transactionId, int subscribeId, int subscribeType,
            String serviceName, String ssi, TlvBufferUtils.TlvConstructor tlvTx,
            TlvBufferUtils.TlvConstructor tlvRx, int subscribeCount, int subscribeTtl)
                    throws JSONException {
        SubscribeData subscribeData = new SubscribeData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(ssi)
                .setTxFilter(tlvTx.getArray(), tlvTx.getActualLength())
                .setRxFilter(tlvRx.getArray(), tlvRx.getActualLength()).build();

        SubscribeSettings subscribeSettings = new SubscribeSettings.Builder()
                .setSubscribeType(subscribeType).setSubscribeCount(subscribeCount)
                .setTtlSec(subscribeTtl).build();

        mDut.subscribe(transactionId, subscribeId, subscribeData, subscribeSettings);

        verify(mNanHalMock).subscribeHalMockNative(eq(transactionId), mArgs.capture());

        Bundle argsData = HalMockUtils.convertJsonToBundle(mArgs.getValue());

        collector.checkThat("subscribe_id", argsData.getInt("subscribe_id"), equalTo(subscribeId));
        collector.checkThat("ttl", argsData.getInt("ttl"), equalTo(subscribeTtl));
        collector.checkThat("period", argsData.getInt("period"), equalTo(500));
        collector.checkThat("subscribe_type", argsData.getInt("subscribe_type"),
                equalTo(subscribeType));
        collector.checkThat("serviceResponseFilter", argsData.getInt("serviceResponseFilter"),
                equalTo(1));
        collector.checkThat("serviceResponseInclude", argsData.getInt("serviceResponseInclude"),
                equalTo(1));
        collector.checkThat("useServiceResponseFilter", argsData.getInt("useServiceResponseFilter"),
                equalTo(1));
        collector.checkThat("ssiRequiredForMatchIndication",
                argsData.getInt("ssiRequiredForMatchIndication"), equalTo(0));
        collector.checkThat("subscribe_match_indicator",
                argsData.getInt("subscribe_match_indicator"), equalTo(0));
        collector.checkThat("subscribe_count", argsData.getInt("subscribe_count"),
                equalTo(subscribeCount));
        collector.checkThat("service_name_len", argsData.getInt("service_name_len"),
                equalTo(serviceName.length()));
        collector.checkThat("service_name", argsData.getByteArray("service_name"),
                equalTo(serviceName.getBytes()));
        collector.checkThat("service_specific_info_len",
                argsData.getInt("service_specific_info_len"), equalTo(serviceName.length()));
        collector.checkThat("service_specific_info", argsData.getByteArray("service_specific_info"),
                equalTo(ssi.getBytes()));
        collector.checkThat("rx_match_filter_len", argsData.getInt("rx_match_filter_len"),
                equalTo(tlvRx.getActualLength()));
        collector.checkThat("rx_match_filter", argsData.getByteArray("rx_match_filter"),
                equalTo(Arrays.copyOf(tlvRx.getArray(), tlvRx.getActualLength())));
        collector.checkThat("tx_match_filter_len", argsData.getInt("tx_match_filter_len"),
                equalTo(tlvTx.getActualLength()));
        collector.checkThat("tx_match_filter", argsData.getByteArray("tx_match_filter"),
                equalTo(Arrays.copyOf(tlvTx.getArray(), tlvTx.getActualLength())));
        collector.checkThat("rssi_threshold_flag", argsData.getInt("rssi_threshold_flag"),
                equalTo(0));
        collector.checkThat("connmap", argsData.getInt("connmap"), equalTo(0));
        collector.checkThat("num_intf_addr_present", argsData.getInt("num_intf_addr_present"),
                equalTo(0));
    }

    private static void installMockNanStateManager(WifiNanStateManager nanStateManager)
            throws Exception {
        Field field = WifiNanStateManager.class.getDeclaredField("sNanStateManagerSingleton");
        field.setAccessible(true);
        field.set(null, nanStateManager);
    }
}
