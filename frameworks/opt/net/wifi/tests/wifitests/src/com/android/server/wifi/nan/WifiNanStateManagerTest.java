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
import static org.hamcrest.core.IsNull.nullValue;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.anyShort;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.verifyZeroInteractions;

import android.net.wifi.nan.ConfigRequest;
import android.net.wifi.nan.IWifiNanEventListener;
import android.net.wifi.nan.IWifiNanSessionListener;
import android.net.wifi.nan.PublishData;
import android.net.wifi.nan.PublishSettings;
import android.net.wifi.nan.SubscribeData;
import android.net.wifi.nan.SubscribeSettings;
import android.net.wifi.nan.WifiNanEventListener;
import android.net.wifi.nan.WifiNanSessionListener;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.SparseArray;

import com.android.server.wifi.MockLooper;

import libcore.util.HexEncoding;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ErrorCollector;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;

/**
 * Unit test harness for WifiNanStateManager.
 */
@SmallTest
public class WifiNanStateManagerTest {
    private MockLooper mMockLooper;
    private WifiNanStateManager mDut;
    @Mock private WifiNanNative mMockNative;

    @Rule
    public ErrorCollector collector = new ErrorCollector();

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        mMockLooper = new MockLooper();

        mDut = installNewNanStateManagerAndResetState();
        mDut.start(mMockLooper.getLooper());

        installMockWifiNanNative(mMockNative);
    }

    @Test
    public void testNanEventsDelivered() throws Exception {
        final int uid = 1005;
        final int clusterLow1 = 5;
        final int clusterHigh1 = 100;
        final int masterPref1 = 111;
        final int clusterLow2 = 7;
        final int clusterHigh2 = 155;
        final int masterPref2 = 0;
        final int reason = WifiNanSessionListener.FAIL_REASON_NO_RESOURCES;
        final byte[] someMac = HexEncoding.decode("000102030405".toCharArray(), false);

        ConfigRequest configRequest1 = new ConfigRequest.Builder().setClusterLow(clusterLow1)
                .setClusterHigh(clusterHigh1).setMasterPreference(masterPref1).build();

        ConfigRequest configRequest2 = new ConfigRequest.Builder().setClusterLow(clusterLow2)
                .setClusterHigh(clusterHigh2).setMasterPreference(masterPref2).build();

        IWifiNanEventListener mockListener = mock(IWifiNanEventListener.class);
        ArgumentCaptor<Short> transactionId = ArgumentCaptor.forClass(Short.class);
        InOrder inOrder = inOrder(mockListener, mMockNative);

        mDut.connect(uid, mockListener,
                WifiNanEventListener.LISTEN_CONFIG_COMPLETED
                        | WifiNanEventListener.LISTEN_CONFIG_FAILED
                        | WifiNanEventListener.LISTEN_IDENTITY_CHANGED
                        | WifiNanEventListener.LISTEN_NAN_DOWN);
        mDut.requestConfig(uid, configRequest1);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).enableAndConfigure(transactionId.capture(), eq(configRequest1));
        short transactionId1 = transactionId.getValue();

        mDut.requestConfig(uid, configRequest2);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).enableAndConfigure(transactionId.capture(), eq(configRequest2));
        short transactionId2 = transactionId.getValue();

        mDut.onClusterChange(WifiNanClientState.CLUSTER_CHANGE_EVENT_STARTED, someMac);
        mDut.onConfigCompleted(transactionId1);
        mDut.onConfigFailed(transactionId2, reason);
        mDut.onInterfaceAddressChange(someMac);
        mDut.onNanDown(reason);
        mMockLooper.dispatchAll();

        inOrder.verify(mockListener).onIdentityChanged();
        inOrder.verify(mockListener).onConfigCompleted(configRequest1);
        inOrder.verify(mockListener).onConfigFailed(configRequest2, reason);
        inOrder.verify(mockListener).onIdentityChanged();
        inOrder.verify(mockListener).onNanDown(reason);
        verifyNoMoreInteractions(mockListener);

        validateInternalTransactionInfoCleanedUp(transactionId1);
        validateInternalTransactionInfoCleanedUp(transactionId2);
    }

    @Test
    public void testNanEventsNotDelivered() throws Exception {
        final int uid = 1005;
        final int clusterLow1 = 5;
        final int clusterHigh1 = 100;
        final int masterPref1 = 111;
        final int clusterLow2 = 7;
        final int clusterHigh2 = 155;
        final int masterPref2 = 0;
        final int reason = WifiNanSessionListener.FAIL_REASON_NO_RESOURCES;
        final byte[] someMac = HexEncoding.decode("000102030405".toCharArray(), false);

        ConfigRequest configRequest1 = new ConfigRequest.Builder().setClusterLow(clusterLow1)
                .setClusterHigh(clusterHigh1).setMasterPreference(masterPref1).build();

        ConfigRequest configRequest2 = new ConfigRequest.Builder().setClusterLow(clusterLow2)
                .setClusterHigh(clusterHigh2).setMasterPreference(masterPref2).build();

        IWifiNanEventListener mockListener = mock(IWifiNanEventListener.class);
        ArgumentCaptor<Short> transactionId = ArgumentCaptor.forClass(Short.class);
        InOrder inOrder = inOrder(mockListener, mMockNative);

        mDut.connect(uid, mockListener, 0);
        mDut.requestConfig(uid, configRequest1);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).enableAndConfigure(transactionId.capture(), eq(configRequest1));
        short transactionId1 = transactionId.getValue();

        mDut.requestConfig(uid, configRequest2);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).enableAndConfigure(transactionId.capture(), eq(configRequest2));
        short transactionId2 = transactionId.getValue();

        mDut.onClusterChange(WifiNanClientState.CLUSTER_CHANGE_EVENT_JOINED, someMac);
        mDut.onConfigCompleted(transactionId1);
        mDut.onConfigFailed(transactionId2, reason);
        mDut.onInterfaceAddressChange(someMac);
        mDut.onNanDown(reason);
        mMockLooper.dispatchAll();

        verifyZeroInteractions(mockListener);

        validateInternalTransactionInfoCleanedUp(transactionId1);
        validateInternalTransactionInfoCleanedUp(transactionId2);
    }

    @Test
    public void testPublish() throws Exception {
        final int uid = 1005;
        final int sessionId = 20;
        final String serviceName = "some-service-name";
        final String ssi = "some much longer and more arbitrary data";
        final int publishCount = 7;
        final int reasonFail = WifiNanSessionListener.FAIL_REASON_NO_RESOURCES;
        final int reasonTerminate = WifiNanSessionListener.TERMINATE_REASON_DONE;
        final int publishId1 = 15;
        final int publishId2 = 22;

        PublishData publishData = new PublishData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(ssi).build();

        PublishSettings publishSettings = new PublishSettings.Builder()
                .setPublishType(PublishSettings.PUBLISH_TYPE_UNSOLICITED)
                .setPublishCount(publishCount).build();

        IWifiNanSessionListener mockListener = mock(IWifiNanSessionListener.class);
        ArgumentCaptor<Short> transactionId = ArgumentCaptor.forClass(Short.class);
        InOrder inOrder = inOrder(mockListener, mMockNative);

        int allEvents = WifiNanSessionListener.LISTEN_PUBLISH_FAIL
                | WifiNanSessionListener.LISTEN_PUBLISH_TERMINATED
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_FAIL
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_TERMINATED
                | WifiNanSessionListener.LISTEN_MATCH
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_SUCCESS
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_FAIL
                | WifiNanSessionListener.LISTEN_MESSAGE_RECEIVED;

        mDut.connect(uid, null, 0);
        mDut.createSession(uid, sessionId, mockListener, allEvents);

        // publish - fail
        mDut.publish(uid, sessionId, publishData, publishSettings);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).publish(transactionId.capture(), eq(0), eq(publishData),
                eq(publishSettings));

        mDut.onPublishFail(transactionId.getValue(), reasonFail);
        mMockLooper.dispatchAll();

        inOrder.verify(mockListener).onPublishFail(reasonFail);
        validateInternalTransactionInfoCleanedUp(transactionId.getValue());

        // publish - success/terminate
        mDut.publish(uid, sessionId, publishData, publishSettings);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).publish(transactionId.capture(), eq(0), eq(publishData),
                eq(publishSettings));

        mDut.onPublishSuccess(transactionId.getValue(), publishId1);
        mMockLooper.dispatchAll();

        mDut.onPublishTerminated(publishId1, reasonTerminate);
        mMockLooper.dispatchAll();

        inOrder.verify(mockListener).onPublishTerminated(reasonTerminate);
        validateInternalTransactionInfoCleanedUp(transactionId.getValue());

        // re-publish
        mDut.publish(uid, sessionId, publishData, publishSettings);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).publish(transactionId.capture(), eq(0), eq(publishData),
                eq(publishSettings));

        mDut.onPublishSuccess(transactionId.getValue(), publishId2);
        mDut.publish(uid, sessionId, publishData, publishSettings);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        inOrder.verify(mMockNative).publish(transactionId.capture(), eq(publishId2),
                eq(publishData), eq(publishSettings));
        verifyNoMoreInteractions(mockListener, mMockNative);
    }

    @Test
    public void testPublishNoCallbacks() throws Exception {
        final int uid = 1005;
        final int sessionId = 20;
        final String serviceName = "some-service-name";
        final String ssi = "some much longer and more arbitrary data";
        final int publishCount = 7;
        final int reasonFail = WifiNanSessionListener.FAIL_REASON_NO_RESOURCES;
        final int reasonTerminate = WifiNanSessionListener.TERMINATE_REASON_DONE;
        final int publishId = 15;

        PublishData publishData = new PublishData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(ssi).build();

        PublishSettings publishSettings = new PublishSettings.Builder()
                .setPublishType(PublishSettings.PUBLISH_TYPE_UNSOLICITED)
                .setPublishCount(publishCount).build();

        IWifiNanSessionListener mockListener = mock(IWifiNanSessionListener.class);
        ArgumentCaptor<Short> transactionId = ArgumentCaptor.forClass(Short.class);
        InOrder inOrder = inOrder(mockListener, mMockNative);

        int allEvents = WifiNanSessionListener.LISTEN_PUBLISH_FAIL
                | WifiNanSessionListener.LISTEN_PUBLISH_TERMINATED
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_FAIL
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_TERMINATED
                | WifiNanSessionListener.LISTEN_MATCH
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_SUCCESS
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_FAIL
                | WifiNanSessionListener.LISTEN_MESSAGE_RECEIVED;

        mDut.connect(uid, null, 0);
        mDut.createSession(uid, sessionId, mockListener,
                allEvents & ~WifiNanSessionListener.LISTEN_PUBLISH_FAIL
                        & ~WifiNanSessionListener.LISTEN_PUBLISH_TERMINATED);

        // publish - fail
        mDut.publish(uid, sessionId, publishData, publishSettings);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).publish(transactionId.capture(), eq(0), eq(publishData),
                eq(publishSettings));

        mDut.onPublishFail(transactionId.getValue(), reasonFail);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());

        // publish - success/terminate
        mDut.publish(uid, sessionId, publishData, publishSettings);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).publish(transactionId.capture(), eq(0), eq(publishData),
                eq(publishSettings));

        mDut.onPublishSuccess(transactionId.getValue(), publishId);
        mMockLooper.dispatchAll();

        mDut.onPublishTerminated(publishId, reasonTerminate);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        verifyNoMoreInteractions(mockListener, mMockNative);
    }

    @Test
    public void testSubscribe() throws Exception {
        final int uid = 1005;
        final int sessionId = 20;
        final String serviceName = "some-service-name";
        final String ssi = "some much longer and more arbitrary data";
        final int subscribeCount = 7;
        final int reasonFail = WifiNanSessionListener.FAIL_REASON_NO_RESOURCES;
        final int reasonTerminate = WifiNanSessionListener.TERMINATE_REASON_DONE;
        final int subscribeId1 = 15;
        final int subscribeId2 = 10;

        SubscribeData subscribeData = new SubscribeData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(ssi).build();

        SubscribeSettings subscribeSettings = new SubscribeSettings.Builder()
                .setSubscribeType(SubscribeSettings.SUBSCRIBE_TYPE_PASSIVE)
                .setSubscribeCount(subscribeCount).build();

        IWifiNanSessionListener mockListener = mock(IWifiNanSessionListener.class);
        ArgumentCaptor<Short> transactionId = ArgumentCaptor.forClass(Short.class);
        InOrder inOrder = inOrder(mockListener, mMockNative);

        int allEvents = WifiNanSessionListener.LISTEN_PUBLISH_FAIL
                | WifiNanSessionListener.LISTEN_PUBLISH_TERMINATED
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_FAIL
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_TERMINATED
                | WifiNanSessionListener.LISTEN_MATCH
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_SUCCESS
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_FAIL
                | WifiNanSessionListener.LISTEN_MESSAGE_RECEIVED;

        mDut.connect(uid, null, 0);
        mDut.createSession(uid, sessionId, mockListener, allEvents);

        // subscribe - fail
        mDut.subscribe(uid, sessionId, subscribeData, subscribeSettings);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).subscribe(transactionId.capture(), eq(0), eq(subscribeData),
                eq(subscribeSettings));

        mDut.onSubscribeFail(transactionId.getValue(), reasonFail);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        inOrder.verify(mockListener).onSubscribeFail(reasonFail);

        // subscribe - success/terminate
        mDut.subscribe(uid, sessionId, subscribeData, subscribeSettings);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).subscribe(transactionId.capture(), eq(0), eq(subscribeData),
                eq(subscribeSettings));

        mDut.onSubscribeSuccess(transactionId.getValue(), subscribeId1);
        mMockLooper.dispatchAll();

        mDut.onSubscribeTerminated(subscribeId1, reasonTerminate);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        inOrder.verify(mockListener).onSubscribeTerminated(reasonTerminate);

        // re-subscribe
        mDut.subscribe(uid, sessionId, subscribeData, subscribeSettings);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).subscribe(transactionId.capture(), eq(0), eq(subscribeData),
                eq(subscribeSettings));

        mDut.onSubscribeSuccess(transactionId.getValue(), subscribeId2);
        mDut.subscribe(uid, sessionId, subscribeData, subscribeSettings);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        inOrder.verify(mMockNative).subscribe(transactionId.capture(), eq(subscribeId2),
                eq(subscribeData), eq(subscribeSettings));
        verifyNoMoreInteractions(mockListener, mMockNative);
    }

    @Test
    public void testSubscribeNoCallbacks() throws Exception {
        final int uid = 1005;
        final int sessionId = 20;
        final String serviceName = "some-service-name";
        final String ssi = "some much longer and more arbitrary data";
        final int subscribeCount = 7;
        final int reasonFail = WifiNanSessionListener.FAIL_REASON_NO_RESOURCES;
        final int reasonTerminate = WifiNanSessionListener.TERMINATE_REASON_DONE;
        final int subscribeId = 15;

        SubscribeData subscribeData = new SubscribeData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(ssi).build();

        SubscribeSettings subscribeSettings = new SubscribeSettings.Builder()
                .setSubscribeType(SubscribeSettings.SUBSCRIBE_TYPE_PASSIVE)
                .setSubscribeCount(subscribeCount).build();

        IWifiNanSessionListener mockListener = mock(IWifiNanSessionListener.class);
        ArgumentCaptor<Short> transactionId = ArgumentCaptor.forClass(Short.class);
        InOrder inOrder = inOrder(mockListener, mMockNative);

        int allEvents = WifiNanSessionListener.LISTEN_PUBLISH_FAIL
                | WifiNanSessionListener.LISTEN_PUBLISH_TERMINATED
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_FAIL
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_TERMINATED
                | WifiNanSessionListener.LISTEN_MATCH
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_SUCCESS
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_FAIL
                | WifiNanSessionListener.LISTEN_MESSAGE_RECEIVED;

        mDut.connect(uid, null, 0);
        mDut.createSession(uid, sessionId, mockListener,
                allEvents & ~WifiNanSessionListener.LISTEN_SUBSCRIBE_FAIL
                        & ~WifiNanSessionListener.LISTEN_SUBSCRIBE_TERMINATED);

        // subscribe - fail
        mDut.subscribe(uid, sessionId, subscribeData, subscribeSettings);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).subscribe(transactionId.capture(), eq(0), eq(subscribeData),
                eq(subscribeSettings));

        mDut.onSubscribeFail(transactionId.getValue(), reasonFail);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());

        // subscribe - success/terminate
        mDut.subscribe(uid, sessionId, subscribeData, subscribeSettings);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).subscribe(transactionId.capture(), eq(0), eq(subscribeData),
                eq(subscribeSettings));

        mDut.onSubscribeSuccess(transactionId.getValue(), subscribeId);
        mMockLooper.dispatchAll();

        mDut.onSubscribeTerminated(subscribeId, reasonTerminate);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        verifyNoMoreInteractions(mockListener, mMockNative);
    }

    @Test
    public void testMatchAndMessages() throws Exception {
        final int uid = 1005;
        final int sessionId = 20;
        final String serviceName = "some-service-name";
        final String ssi = "some much longer and more arbitrary data";
        final int subscribeCount = 7;
        final int reasonFail = WifiNanSessionListener.FAIL_REASON_NO_RESOURCES;
        final int subscribeId = 15;
        final int requestorId = 22;
        final byte[] peerMac = HexEncoding.decode("060708090A0B".toCharArray(), false);
        final String peerSsi = "some peer ssi data";
        final String peerMatchFilter = "filter binary array represented as string";
        final String peerMsg = "some message from peer";
        final int messageId = 6948;

        SubscribeData subscribeData = new SubscribeData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(ssi).build();

        SubscribeSettings subscribeSettings = new SubscribeSettings.Builder()
                .setSubscribeType(SubscribeSettings.SUBSCRIBE_TYPE_PASSIVE)
                .setSubscribeCount(subscribeCount).build();

        IWifiNanSessionListener mockListener = mock(IWifiNanSessionListener.class);
        ArgumentCaptor<Short> transactionId = ArgumentCaptor.forClass(Short.class);
        InOrder inOrder = inOrder(mockListener, mMockNative);

        int allEvents = WifiNanSessionListener.LISTEN_PUBLISH_FAIL
                | WifiNanSessionListener.LISTEN_PUBLISH_TERMINATED
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_FAIL
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_TERMINATED
                | WifiNanSessionListener.LISTEN_MATCH
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_SUCCESS
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_FAIL
                | WifiNanSessionListener.LISTEN_MESSAGE_RECEIVED;

        mDut.connect(uid, null, 0);
        mDut.createSession(uid, sessionId, mockListener, allEvents);
        mDut.subscribe(uid, sessionId, subscribeData, subscribeSettings);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).subscribe(transactionId.capture(), eq(0), eq(subscribeData),
                eq(subscribeSettings));

        mDut.onSubscribeSuccess(transactionId.getValue(), subscribeId);
        mDut.onMatch(subscribeId, requestorId, peerMac, peerSsi.getBytes(), peerSsi.length(),
                peerMatchFilter.getBytes(), peerMatchFilter.length());
        mDut.onMessageReceived(subscribeId, requestorId, peerMac, peerMsg.getBytes(),
                peerMsg.length());
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        inOrder.verify(mockListener).onMatch(requestorId, peerSsi.getBytes(), peerSsi.length(),
                peerMatchFilter.getBytes(), peerMatchFilter.length());
        inOrder.verify(mockListener).onMessageReceived(requestorId, peerMsg.getBytes(),
                peerMsg.length());

        mDut.sendMessage(uid, sessionId, requestorId, ssi.getBytes(), ssi.length(), messageId);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).sendMessage(transactionId.capture(), eq(subscribeId),
                eq(requestorId), eq(peerMac), eq(ssi.getBytes()), eq(ssi.length()));

        mDut.onMessageSendFail(transactionId.getValue(), reasonFail);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        inOrder.verify(mockListener).onMessageSendFail(messageId, reasonFail);

        mDut.sendMessage(uid, sessionId, requestorId, ssi.getBytes(), ssi.length(), messageId);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).sendMessage(transactionId.capture(), eq(subscribeId),
                eq(requestorId), eq(peerMac), eq(ssi.getBytes()), eq(ssi.length()));

        mDut.onMessageSendSuccess(transactionId.getValue());
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        inOrder.verify(mockListener).onMessageSendSuccess(messageId);

        verifyNoMoreInteractions(mockListener, mMockNative);
    }

    /**
     * Summary: in a single publish session interact with multiple peers
     * (different MAC addresses).
     */
    @Test
    public void testMultipleMessageSources() throws Exception {
        final int uid = 300;
        final int clusterLow = 7;
        final int clusterHigh = 7;
        final int masterPref = 0;
        final int sessionId = 26;
        final String serviceName = "some-service-name";
        final int publishId = 88;
        final int peerId1 = 568;
        final int peerId2 = 873;
        final byte[] peerMac1 = HexEncoding.decode("000102030405".toCharArray(), false);
        final byte[] peerMac2 = HexEncoding.decode("060708090A0B".toCharArray(), false);
        final String msgFromPeer1 = "hey from 000102...";
        final String msgFromPeer2 = "hey from 0607...";
        final String msgToPeer1 = "hey there 000102...";
        final String msgToPeer2 = "hey there 0506...";
        final int msgToPeerId1 = 546;
        final int msgToPeerId2 = 9654;
        final int reason = WifiNanSessionListener.FAIL_REASON_OTHER;

        ConfigRequest configRequest = new ConfigRequest.Builder().setClusterLow(clusterLow)
                .setClusterHigh(clusterHigh).setMasterPreference(masterPref).build();

        PublishData publishData = new PublishData.Builder().setServiceName(serviceName).build();

        PublishSettings publishSettings = new PublishSettings.Builder()
                .setPublishType(PublishSettings.PUBLISH_TYPE_UNSOLICITED).build();

        ArgumentCaptor<Short> transactionId = ArgumentCaptor.forClass(Short.class);
        IWifiNanEventListener mockListener = mock(IWifiNanEventListener.class);
        IWifiNanSessionListener mockSessionListener = mock(IWifiNanSessionListener.class);
        InOrder inOrder = inOrder(mMockNative, mockListener, mockSessionListener);

        int allEvents = WifiNanEventListener.LISTEN_CONFIG_COMPLETED
                | WifiNanEventListener.LISTEN_CONFIG_FAILED
                | WifiNanEventListener.LISTEN_IDENTITY_CHANGED
                | WifiNanEventListener.LISTEN_NAN_DOWN;

        int allSessionEvents = WifiNanSessionListener.LISTEN_PUBLISH_FAIL
                | WifiNanSessionListener.LISTEN_PUBLISH_TERMINATED
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_FAIL
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_TERMINATED
                | WifiNanSessionListener.LISTEN_MATCH
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_SUCCESS
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_FAIL
                | WifiNanSessionListener.LISTEN_MESSAGE_RECEIVED;

        mDut.connect(uid, mockListener, allEvents);
        mDut.requestConfig(uid, configRequest);
        mDut.createSession(uid, sessionId, mockSessionListener, allSessionEvents);
        mDut.publish(uid, sessionId, publishData, publishSettings);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).enableAndConfigure(transactionId.capture(), eq(configRequest));
        short transactionIdConfig = transactionId.getValue();

        inOrder.verify(mMockNative).publish(transactionId.capture(), eq(0), eq(publishData),
                eq(publishSettings));
        short transactionIdPublish = transactionId.getValue();

        mDut.onConfigCompleted(transactionIdConfig);
        mDut.onPublishSuccess(transactionIdPublish, publishId);
        mDut.onMessageReceived(publishId, peerId1, peerMac1, msgFromPeer1.getBytes(),
                msgFromPeer1.length());
        mDut.onMessageReceived(publishId, peerId2, peerMac2, msgFromPeer2.getBytes(),
                msgFromPeer2.length());
        mDut.sendMessage(uid, sessionId, peerId2, msgToPeer2.getBytes(), msgToPeer2.length(),
                msgToPeerId2);
        mDut.sendMessage(uid, sessionId, peerId1, msgToPeer1.getBytes(), msgToPeer1.length(),
                msgToPeerId1);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionIdConfig);
        validateInternalTransactionInfoCleanedUp(transactionIdPublish);
        inOrder.verify(mockListener).onConfigCompleted(configRequest);
        inOrder.verify(mockSessionListener).onMessageReceived(peerId1, msgFromPeer1.getBytes(),
                msgFromPeer1.length());
        inOrder.verify(mockSessionListener).onMessageReceived(peerId2, msgFromPeer2.getBytes(),
                msgFromPeer2.length());
        inOrder.verify(mMockNative).sendMessage(transactionId.capture(), eq(publishId), eq(peerId2),
                eq(peerMac2), eq(msgToPeer2.getBytes()), eq(msgToPeer2.length()));
        short transactionIdMsg2 = transactionId.getValue();
        inOrder.verify(mMockNative).sendMessage(transactionId.capture(), eq(publishId), eq(peerId1),
                eq(peerMac1), eq(msgToPeer1.getBytes()), eq(msgToPeer1.length()));
        short transactionIdMsg1 = transactionId.getValue();

        mDut.onMessageSendFail(transactionIdMsg1, reason);
        mDut.onMessageSendSuccess(transactionIdMsg2);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionIdMsg1);
        validateInternalTransactionInfoCleanedUp(transactionIdMsg2);
        inOrder.verify(mockSessionListener).onMessageSendFail(msgToPeerId1, reason);
        inOrder.verify(mockSessionListener).onMessageSendSuccess(msgToPeerId2);
        verifyNoMoreInteractions(mMockNative, mockListener, mockSessionListener);
    }

    /**
     * Summary: interact with a peer which changed its identity (MAC address)
     * but which keeps its requestor instance ID. Should be transparent.
     */
    @Test
    public void testMessageWhilePeerChangesIdentity() throws Exception {
        final int uid = 300;
        final int clusterLow = 7;
        final int clusterHigh = 7;
        final int masterPref = 0;
        final int sessionId = 26;
        final String serviceName = "some-service-name";
        final int publishId = 88;
        final int peerId = 568;
        final byte[] peerMacOrig = HexEncoding.decode("000102030405".toCharArray(), false);
        final byte[] peerMacLater = HexEncoding.decode("060708090A0B".toCharArray(), false);
        final String msgFromPeer1 = "hey from 000102...";
        final String msgFromPeer2 = "hey from 0607...";
        final String msgToPeer1 = "hey there 000102...";
        final String msgToPeer2 = "hey there 0506...";
        final int msgToPeerId1 = 546;
        final int msgToPeerId2 = 9654;
        ConfigRequest configRequest = new ConfigRequest.Builder().setClusterLow(clusterLow)
                .setClusterHigh(clusterHigh).setMasterPreference(masterPref).build();

        PublishData publishData = new PublishData.Builder().setServiceName(serviceName).build();

        PublishSettings publishSettings = new PublishSettings.Builder()
                .setPublishType(PublishSettings.PUBLISH_TYPE_UNSOLICITED).build();

        ArgumentCaptor<Short> transactionId = ArgumentCaptor.forClass(Short.class);
        IWifiNanEventListener mockListener = mock(IWifiNanEventListener.class);
        IWifiNanSessionListener mockSessionListener = mock(IWifiNanSessionListener.class);
        InOrder inOrder = inOrder(mMockNative, mockListener, mockSessionListener);

        int allEvents = WifiNanEventListener.LISTEN_CONFIG_COMPLETED
                | WifiNanEventListener.LISTEN_CONFIG_FAILED
                | WifiNanEventListener.LISTEN_IDENTITY_CHANGED
                | WifiNanEventListener.LISTEN_NAN_DOWN;

        int allSessionEvents = WifiNanSessionListener.LISTEN_PUBLISH_FAIL
                | WifiNanSessionListener.LISTEN_PUBLISH_TERMINATED
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_FAIL
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_TERMINATED
                | WifiNanSessionListener.LISTEN_MATCH
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_SUCCESS
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_FAIL
                | WifiNanSessionListener.LISTEN_MESSAGE_RECEIVED;

        mDut.connect(uid, mockListener, allEvents);
        mDut.requestConfig(uid, configRequest);
        mDut.createSession(uid, sessionId, mockSessionListener, allSessionEvents);
        mDut.publish(uid, sessionId, publishData, publishSettings);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).enableAndConfigure(transactionId.capture(), eq(configRequest));
        short transactionIdConfig = transactionId.getValue();

        inOrder.verify(mMockNative).publish(transactionId.capture(), eq(0), eq(publishData),
                eq(publishSettings));
        short transactionIdPublish = transactionId.getValue();

        mDut.onConfigCompleted(transactionIdConfig);
        mDut.onPublishSuccess(transactionIdPublish, publishId);
        mDut.onMessageReceived(publishId, peerId, peerMacOrig, msgFromPeer1.getBytes(),
                msgFromPeer1.length());
        mDut.sendMessage(uid, sessionId, peerId, msgToPeer1.getBytes(), msgToPeer1.length(),
                msgToPeerId1);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionIdConfig);
        validateInternalTransactionInfoCleanedUp(transactionIdPublish);
        inOrder.verify(mockListener).onConfigCompleted(configRequest);
        inOrder.verify(mockSessionListener).onMessageReceived(peerId, msgFromPeer1.getBytes(),
                msgFromPeer1.length());
        inOrder.verify(mMockNative).sendMessage(transactionId.capture(), eq(publishId), eq(peerId),
                eq(peerMacOrig), eq(msgToPeer1.getBytes()), eq(msgToPeer1.length()));
        short transactionIdMsg = transactionId.getValue();

        mDut.onMessageSendSuccess(transactionIdMsg);
        mDut.onMessageReceived(publishId, peerId, peerMacLater, msgFromPeer2.getBytes(),
                msgFromPeer2.length());
        mDut.sendMessage(uid, sessionId, peerId, msgToPeer2.getBytes(), msgToPeer2.length(),
                msgToPeerId2);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionIdMsg);
        inOrder.verify(mockSessionListener).onMessageSendSuccess(msgToPeerId1);
        inOrder.verify(mockSessionListener).onMessageReceived(peerId, msgFromPeer2.getBytes(),
                msgFromPeer2.length());
        inOrder.verify(mMockNative).sendMessage(transactionId.capture(), eq(publishId), eq(peerId),
                eq(peerMacLater), eq(msgToPeer2.getBytes()), eq(msgToPeer2.length()));
        transactionIdMsg = transactionId.getValue();

        mDut.onMessageSendSuccess(transactionIdMsg);
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionIdMsg);
        inOrder.verify(mockSessionListener).onMessageSendSuccess(msgToPeerId2);

        verifyNoMoreInteractions(mMockNative, mockListener, mockSessionListener);
    }

    @Test
    public void testConfigs() throws Exception {
        final int uid1 = 9999;
        final int clusterLow1 = 5;
        final int clusterHigh1 = 100;
        final int masterPref1 = 111;
        final int uid2 = 1001;
        final boolean support5g2 = true;
        final int clusterLow2 = 7;
        final int clusterHigh2 = 155;
        final int masterPref2 = 0;
        final int uid3 = 55;

        ArgumentCaptor<Short> transactionId = ArgumentCaptor.forClass(Short.class);
        ArgumentCaptor<ConfigRequest> crCapture = ArgumentCaptor.forClass(ConfigRequest.class);

        ConfigRequest configRequest1 = new ConfigRequest.Builder().setClusterLow(clusterLow1)
                .setClusterHigh(clusterHigh1).setMasterPreference(masterPref1).build();

        ConfigRequest configRequest2 = new ConfigRequest.Builder().setSupport5gBand(support5g2)
                .setClusterLow(clusterLow2).setClusterHigh(clusterHigh2)
                .setMasterPreference(masterPref2).build();

        ConfigRequest configRequest3 = new ConfigRequest.Builder().build();

        IWifiNanEventListener mockListener1 = mock(IWifiNanEventListener.class);
        IWifiNanEventListener mockListener2 = mock(IWifiNanEventListener.class);
        IWifiNanEventListener mockListener3 = mock(IWifiNanEventListener.class);

        InOrder inOrder = inOrder(mMockNative, mockListener1, mockListener2, mockListener3);

        mDut.connect(uid1, mockListener1, WifiNanEventListener.LISTEN_CONFIG_COMPLETED);
        mDut.requestConfig(uid1, configRequest1);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).enableAndConfigure(transactionId.capture(),
                crCapture.capture());
        collector.checkThat("merge: stage 0", configRequest1, equalTo(crCapture.getValue()));

        mDut.onConfigCompleted(transactionId.getValue());
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        inOrder.verify(mockListener1).onConfigCompleted(configRequest1);

        mDut.connect(uid2, mockListener2, WifiNanEventListener.LISTEN_CONFIG_COMPLETED);
        mDut.requestConfig(uid2, configRequest2);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).enableAndConfigure(transactionId.capture(),
                crCapture.capture());
        collector.checkThat("merge: stage 1: support 5g", crCapture.getValue().mSupport5gBand,
                equalTo(true));
        collector.checkThat("merge: stage 1: master pref", crCapture.getValue().mMasterPreference,
                equalTo(Math.max(masterPref1, masterPref2)));
        collector.checkThat("merge: stage 1: cluster low", crCapture.getValue().mClusterLow,
                equalTo(Math.min(clusterLow1, clusterLow2)));
        collector.checkThat("merge: stage 1: cluster high", crCapture.getValue().mClusterHigh,
                equalTo(Math.max(clusterHigh1, clusterHigh2)));

        mDut.onConfigCompleted(transactionId.getValue());
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        inOrder.verify(mockListener1).onConfigCompleted(crCapture.getValue());

        mDut.connect(uid3, mockListener3, WifiNanEventListener.LISTEN_CONFIG_COMPLETED);
        mDut.requestConfig(uid3, configRequest3);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).enableAndConfigure(transactionId.capture(),
                crCapture.capture());
        collector.checkThat("merge: stage 2: support 5g", crCapture.getValue().mSupport5gBand,
                equalTo(true));
        collector.checkThat("merge: stage 2: master pref", crCapture.getValue().mMasterPreference,
                equalTo(Math.max(masterPref1, masterPref2)));
        collector.checkThat("merge: stage 2: cluster low", crCapture.getValue().mClusterLow,
                equalTo(Math.min(clusterLow1, clusterLow2)));
        collector.checkThat("merge: stage 2: cluster high", crCapture.getValue().mClusterHigh,
                equalTo(Math.max(clusterHigh1, clusterHigh2)));

        mDut.onConfigCompleted(transactionId.getValue());
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        inOrder.verify(mockListener1).onConfigCompleted(crCapture.getValue());

        mDut.disconnect(uid2);
        mMockLooper.dispatchAll();

        validateInternalClientInfoCleanedUp(uid2);
        inOrder.verify(mMockNative).enableAndConfigure(transactionId.capture(),
                crCapture.capture());
        collector.checkThat("merge: stage 3", configRequest1, equalTo(crCapture.getValue()));

        mDut.onConfigCompleted(transactionId.getValue());
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        inOrder.verify(mockListener1).onConfigCompleted(crCapture.getValue());

        mDut.disconnect(uid1);
        mMockLooper.dispatchAll();

        validateInternalClientInfoCleanedUp(uid2);
        inOrder.verify(mMockNative).enableAndConfigure(transactionId.capture(),
                crCapture.capture());
        collector.checkThat("merge: stage 4", configRequest3, equalTo(crCapture.getValue()));

        mDut.onConfigCompleted(transactionId.getValue());
        mMockLooper.dispatchAll();

        validateInternalTransactionInfoCleanedUp(transactionId.getValue());
        inOrder.verify(mockListener3).onConfigCompleted(crCapture.getValue());

        mDut.disconnect(uid3);
        mMockLooper.dispatchAll();

        validateInternalClientInfoCleanedUp(uid2);
        inOrder.verify(mMockNative).disable(anyShort());

        verifyNoMoreInteractions(mMockNative);
    }

    /**
     * Summary: disconnect a client while there are pending transactions.
     * Validate that no callbacks are called and that internal state is
     * cleaned-up.
     */
    @Test
    public void testDisconnectWithPendingTransactions() throws Exception {
        final int uid = 125;
        final int clusterLow = 5;
        final int clusterHigh = 100;
        final int masterPref = 111;
        final int sessionId = 20;
        final String serviceName = "some-service-name";
        final String ssi = "some much longer and more arbitrary data";
        final int publishCount = 7;
        final int reason = WifiNanSessionListener.TERMINATE_REASON_DONE;
        final int publishId = 22;

        ConfigRequest configRequest = new ConfigRequest.Builder().setClusterLow(clusterLow)
                .setClusterHigh(clusterHigh).setMasterPreference(masterPref).build();

        PublishData publishData = new PublishData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(ssi).build();

        PublishSettings publishSettings = new PublishSettings.Builder()
                .setPublishType(PublishSettings.PUBLISH_TYPE_UNSOLICITED)
                .setPublishCount(publishCount).build();

        ArgumentCaptor<Short> transactionId = ArgumentCaptor.forClass(Short.class);
        IWifiNanEventListener mockListener = mock(IWifiNanEventListener.class);
        IWifiNanSessionListener mockSessionListener = mock(IWifiNanSessionListener.class);
        InOrder inOrder = inOrder(mMockNative, mockListener, mockSessionListener);

        int allEvents = WifiNanEventListener.LISTEN_CONFIG_COMPLETED
                | WifiNanEventListener.LISTEN_CONFIG_FAILED
                | WifiNanEventListener.LISTEN_IDENTITY_CHANGED
                | WifiNanEventListener.LISTEN_NAN_DOWN;

        int allSessionEvents = WifiNanSessionListener.LISTEN_PUBLISH_FAIL
                | WifiNanSessionListener.LISTEN_PUBLISH_TERMINATED
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_FAIL
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_TERMINATED
                | WifiNanSessionListener.LISTEN_MATCH
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_SUCCESS
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_FAIL
                | WifiNanSessionListener.LISTEN_MESSAGE_RECEIVED;

        mDut.connect(uid, mockListener, allEvents);
        mDut.createSession(uid, sessionId, mockSessionListener, allSessionEvents);
        mDut.requestConfig(uid, configRequest);
        mDut.publish(uid, sessionId, publishData, publishSettings);
        mDut.disconnect(uid);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).enableAndConfigure(transactionId.capture(), eq(configRequest));
        short transactionIdConfig = transactionId.getValue();

        inOrder.verify(mMockNative).publish(transactionId.capture(), eq(0), eq(publishData),
                eq(publishSettings));
        short transactionIdPublish = transactionId.getValue();

        validateInternalClientInfoCleanedUp(uid);
        validateInternalTransactionInfoCleanedUp(transactionIdPublish);

        mDut.onConfigCompleted(transactionIdConfig);
        mDut.onPublishSuccess(transactionIdPublish, publishId);
        mMockLooper.dispatchAll();

        mDut.onPublishTerminated(publishId, reason);
        mMockLooper.dispatchAll();

        verifyZeroInteractions(mockListener, mockSessionListener);
    }

    /**
     * Summary: destroy a session while there are pending transactions. Validate
     * that no callbacks are called and that internal state is cleaned-up.
     */
    @Test
    public void testDestroySessionWithPendingTransactions() throws Exception {
        final int uid = 128;
        final int clusterLow = 15;
        final int clusterHigh = 192;
        final int masterPref = 234;
        final int publishSessionId = 19;
        final int subscribeSessionId = 24;
        final String serviceName = "some-service-name";
        final String ssi = "some much longer and more arbitrary data";
        final int publishCount = 15;
        final int subscribeCount = 22;
        final int reason = WifiNanSessionListener.TERMINATE_REASON_DONE;
        final int publishId = 23;
        final int subscribeId = 55;

        ConfigRequest configRequest = new ConfigRequest.Builder().setClusterLow(clusterLow)
                .setClusterHigh(clusterHigh).setMasterPreference(masterPref).build();

        PublishData publishData = new PublishData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(ssi).build();

        PublishSettings publishSettings = new PublishSettings.Builder()
                .setPublishType(PublishSettings.PUBLISH_TYPE_UNSOLICITED)
                .setPublishCount(publishCount).build();

        SubscribeData subscribeData = new SubscribeData.Builder().setServiceName(serviceName)
                .setServiceSpecificInfo(ssi).build();

        SubscribeSettings subscribeSettings = new SubscribeSettings.Builder()
                .setSubscribeType(SubscribeSettings.SUBSCRIBE_TYPE_PASSIVE)
                .setSubscribeCount(subscribeCount).build();

        ArgumentCaptor<Short> transactionId = ArgumentCaptor.forClass(Short.class);
        IWifiNanEventListener mockListener = mock(IWifiNanEventListener.class);
        IWifiNanSessionListener mockPublishSessionListener = mock(IWifiNanSessionListener.class);
        IWifiNanSessionListener mockSubscribeSessionListener = mock(IWifiNanSessionListener.class);
        InOrder inOrder = inOrder(mMockNative, mockListener, mockPublishSessionListener,
                mockSubscribeSessionListener);

        int allEvents = WifiNanEventListener.LISTEN_CONFIG_COMPLETED
                | WifiNanEventListener.LISTEN_CONFIG_FAILED
                | WifiNanEventListener.LISTEN_IDENTITY_CHANGED
                | WifiNanEventListener.LISTEN_NAN_DOWN;

        int allSessionEvents = WifiNanSessionListener.LISTEN_PUBLISH_FAIL
                | WifiNanSessionListener.LISTEN_PUBLISH_TERMINATED
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_FAIL
                | WifiNanSessionListener.LISTEN_SUBSCRIBE_TERMINATED
                | WifiNanSessionListener.LISTEN_MATCH
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_SUCCESS
                | WifiNanSessionListener.LISTEN_MESSAGE_SEND_FAIL
                | WifiNanSessionListener.LISTEN_MESSAGE_RECEIVED;

        mDut.connect(uid, mockListener, allEvents);
        mDut.requestConfig(uid, configRequest);
        mDut.createSession(uid, publishSessionId, mockPublishSessionListener, allSessionEvents);
        mDut.publish(uid, publishSessionId, publishData, publishSettings);
        mDut.createSession(uid, subscribeSessionId, mockSubscribeSessionListener, allSessionEvents);
        mDut.subscribe(uid, subscribeSessionId, subscribeData, subscribeSettings);
        mDut.destroySession(uid, publishSessionId);
        mMockLooper.dispatchAll();

        inOrder.verify(mMockNative).enableAndConfigure(transactionId.capture(), eq(configRequest));
        short transactionIdConfig = transactionId.getValue();

        inOrder.verify(mMockNative).publish(transactionId.capture(), eq(0), eq(publishData),
                eq(publishSettings));
        short transactionIdPublish = transactionId.getValue();

        inOrder.verify(mMockNative).subscribe(transactionId.capture(), eq(0), eq(subscribeData),
                eq(subscribeSettings));
        short transactionIdSubscribe = transactionId.getValue();

        validateInternalTransactionInfoCleanedUp(transactionIdPublish);

        mDut.onConfigCompleted(transactionIdConfig);
        mDut.onPublishSuccess(transactionIdPublish, publishId);
        mDut.onSubscribeSuccess(transactionIdSubscribe, subscribeId);
        mMockLooper.dispatchAll();

        mDut.onPublishTerminated(publishId, reason);
        mDut.destroySession(uid, subscribeSessionId);
        mMockLooper.dispatchAll();

        inOrder.verify(mockListener).onConfigCompleted(configRequest);
        verifyZeroInteractions(mockPublishSessionListener);
        verifyNoMoreInteractions(mockSubscribeSessionListener);
    }

    @Test
    public void testTransactionIdIncrement() {
        int loopCount = 100;

        short prevId = 0;
        for (int i = 0; i < loopCount; ++i) {
            short id = mDut.createNextTransactionId();
            if (i != 0) {
                assertTrue("Transaction ID incrementing", id > prevId);
            }
            prevId = id;
        }
    }

    /*
     * Tests of internal state of WifiNanStateManager: very limited (not usually
     * a good idea). However, these test that the internal state is cleaned-up
     * appropriately. Alternatively would cause issues with memory leaks or
     * information leak between sessions.
     */

    /**
     * Utility routine used to validate that the internal state is cleaned-up
     * after the specific transaction ID. To be used in every test which
     * involves a transaction.
     *
     * @param transactionId The transaction ID whose state should be erased.
     */
    public void validateInternalTransactionInfoCleanedUp(short transactionId) throws Exception {
        Object info = getInternalPendingTransactionInfo(mDut, transactionId);
        collector.checkThat("Transaction record not cleared up for transactionId=" + transactionId,
                info, nullValue());
    }

    /**
     * Utility routine used to validate that the internal state is cleaned-up
     * after a client is disconnected. To be used in every test which terminates
     * a client.
     *
     * @param uid The ID of the client which should be deleted.
     */
    public void validateInternalClientInfoCleanedUp(int uid) throws Exception {
        WifiNanClientState client = getInternalClientState(mDut, uid);
        collector.checkThat("Client record not cleared up for uid=" + uid, client, nullValue());
    }

    /*
     * Utilities
     */

    private static WifiNanStateManager installNewNanStateManagerAndResetState() throws Exception {
        Constructor<WifiNanStateManager> ctr = WifiNanStateManager.class.getDeclaredConstructor();
        ctr.setAccessible(true);
        WifiNanStateManager nanStateManager = ctr.newInstance();

        Field field = WifiNanStateManager.class.getDeclaredField("sNanStateManagerSingleton");
        field.setAccessible(true);
        field.set(null, nanStateManager);

        return WifiNanStateManager.getInstance();
    }

    private static void installMockWifiNanNative(WifiNanNative obj) throws Exception {
        Field field = WifiNanNative.class.getDeclaredField("sWifiNanNativeSingleton");
        field.setAccessible(true);
        field.set(null, obj);
    }

    private static Object getInternalPendingTransactionInfo(WifiNanStateManager dut,
            short transactionId) throws Exception {
        Field field = WifiNanStateManager.class.getDeclaredField("mPendingResponses");
        field.setAccessible(true);
        @SuppressWarnings("unchecked")
        SparseArray<Object> pendingResponses = (SparseArray<Object>) field.get(dut);

        return pendingResponses.get(transactionId);
    }

    private static WifiNanClientState getInternalClientState(WifiNanStateManager dut,
            int uid) throws Exception {
        Field field = WifiNanStateManager.class.getDeclaredField("mClients");
        field.setAccessible(true);
        @SuppressWarnings("unchecked")
        SparseArray<WifiNanClientState> clients = (SparseArray<WifiNanClientState>) field.get(dut);

        return clients.get(uid);
    }
}

