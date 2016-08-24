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

package com.googlecode.android_scripting.facade.wifi;

import android.app.Service;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.wifi.nan.ConfigRequest;
import android.net.wifi.nan.PublishData;
import android.net.wifi.nan.PublishSettings;
import android.net.wifi.nan.SubscribeData;
import android.net.wifi.nan.SubscribeSettings;
import android.net.wifi.nan.TlvBufferUtils;
import android.net.wifi.nan.WifiNanEventListener;
import android.net.wifi.nan.WifiNanManager;
import android.net.wifi.nan.WifiNanSession;
import android.net.wifi.nan.WifiNanSessionListener;
import android.os.Bundle;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.RemoteException;

import com.googlecode.android_scripting.facade.EventFacade;
import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcParameter;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * WifiNanManager functions.
 */
public class WifiNanManagerFacade extends RpcReceiver {
    private final Service mService;
    private final EventFacade mEventFacade;

    private WifiNanManager mMgr;
    private WifiNanSession mSession;
    private HandlerThread mNanFacadeThread;
    private ConnectivityManager mConnMgr;

    private static TlvBufferUtils.TlvConstructor getFilterData(JSONObject j) throws JSONException {
        if (j == null) {
            return null;
        }

        TlvBufferUtils.TlvConstructor constructor = new TlvBufferUtils.TlvConstructor(0, 1);
        constructor.allocate(255);

        if (j.has("int0")) {
            constructor.putShort(0, (short) j.getInt("int0"));
        }

        if (j.has("int1")) {
            constructor.putShort(0, (short) j.getInt("int1"));
        }

        if (j.has("data0")) {
            constructor.putString(0, j.getString("data0"));
        }

        if (j.has("data1")) {
            constructor.putString(0, j.getString("data1"));
        }

        return constructor;
    }

    private static ConfigRequest getConfigRequest(JSONObject j) throws JSONException {
        if (j == null) {
            return null;
        }

        ConfigRequest.Builder builder = new ConfigRequest.Builder();

        if (j.has("Support5gBand")) {
            builder.setSupport5gBand(j.getBoolean("Support5gBand"));
        }
        if (j.has("MasterPreference")) {
            builder.setMasterPreference(j.getInt("MasterPreference"));
        }
        if (j.has("ClusterLow")) {
            builder.setClusterLow(j.getInt("ClusterLow"));
        }
        if (j.has("ClusterHigh")) {
            builder.setClusterHigh(j.getInt("ClusterHigh"));
        }

        return builder.build();
    }

    private static PublishData getPublishData(JSONObject j) throws JSONException {
        if (j == null) {
            return null;
        }

        PublishData.Builder builder = new PublishData.Builder();

        if (j.has("ServiceName")) {
            builder.setServiceName(j.getString("ServiceName"));
        }

        if (j.has("ServiceSpecificInfo")) {
            String ssi = j.getString("ServiceSpecificInfo");
            builder.setServiceSpecificInfo(ssi.getBytes(), ssi.length());
        }

        if (j.has("TxFilter")) {
            TlvBufferUtils.TlvConstructor constructor = getFilterData(j.getJSONObject("TxFilter"));
            builder.setTxFilter(constructor.getArray(), constructor.getActualLength());
        }

        if (j.has("RxFilter")) {
            TlvBufferUtils.TlvConstructor constructor = getFilterData(j.getJSONObject("RxFilter"));
            builder.setRxFilter(constructor.getArray(), constructor.getActualLength());
        }

        return builder.build();
    }

    private static PublishSettings getPublishSettings(JSONObject j) throws JSONException {
        if (j == null) {
            return null;
        }

        PublishSettings.Builder builder = new PublishSettings.Builder();

        if (j.has("PublishType")) {
            builder.setPublishType(j.getInt("PublishType"));
        }
        if (j.has("PublishCount")) {
            builder.setPublishCount(j.getInt("PublishCount"));
        }
        if (j.has("TtlSec")) {
            builder.setTtlSec(j.getInt("TtlSec"));
        }

        return builder.build();
    }

    private static SubscribeData getSubscribeData(JSONObject j) throws JSONException {
        if (j == null) {
            return null;
        }

        SubscribeData.Builder builder = new SubscribeData.Builder();

        if (j.has("ServiceName")) {
            builder.setServiceName(j.getString("ServiceName"));
        }

        if (j.has("ServiceSpecificInfo")) {
            String ssi = j.getString("ServiceSpecificInfo");
            builder.setServiceSpecificInfo(ssi);
        }

        if (j.has("TxFilter")) {
            TlvBufferUtils.TlvConstructor constructor = getFilterData(j.getJSONObject("TxFilter"));
            builder.setTxFilter(constructor.getArray(), constructor.getActualLength());
        }

        if (j.has("RxFilter")) {
            TlvBufferUtils.TlvConstructor constructor = getFilterData(j.getJSONObject("RxFilter"));
            builder.setRxFilter(constructor.getArray(), constructor.getActualLength());
        }

        return builder.build();
    }

    private static SubscribeSettings getSubscribeSettings(JSONObject j) throws JSONException {
        if (j == null) {
            return null;
        }

        SubscribeSettings.Builder builder = new SubscribeSettings.Builder();

        if (j.has("SubscribeType")) {
            builder.setSubscribeType(j.getInt("SubscribeType"));
        }
        if (j.has("SubscribeCount")) {
            builder.setSubscribeCount(j.getInt("SubscribeCount"));
        }
        if (j.has("TtlSec")) {
            builder.setTtlSec(j.getInt("TtlSec"));
        }

        return builder.build();
    }

    public WifiNanManagerFacade(FacadeManager manager) {
        super(manager);
        mService = manager.getService();

        mNanFacadeThread = new HandlerThread("nanFacadeThread");
        mNanFacadeThread.start();

        mMgr = (WifiNanManager) mService.getSystemService(Context.WIFI_NAN_SERVICE);
        mMgr.connect(new NanEventListenerPostsEvents(mNanFacadeThread.getLooper()),
                WifiNanEventListener.LISTEN_CONFIG_COMPLETED
                        | WifiNanEventListener.LISTEN_CONFIG_FAILED
                        | WifiNanEventListener.LISTEN_NAN_DOWN
                        | WifiNanEventListener.LISTEN_IDENTITY_CHANGED);

        mConnMgr = (ConnectivityManager) mService.getSystemService(Context.CONNECTIVITY_SERVICE);

        mEventFacade = manager.getReceiver(EventFacade.class);
    }

    @Override
    public void shutdown() {
    }

    @Rpc(description = "Start NAN.")
    public void wifiNanEnable(@RpcParameter(name = "nanConfig") JSONObject nanConfig)
            throws RemoteException, JSONException {
        mMgr.requestConfig(getConfigRequest(nanConfig));
    }

    @Rpc(description = "Stop NAN.")
    public void wifiNanDisable() throws RemoteException, JSONException {
        mMgr.disconnect();
    }

    @Rpc(description = "Publish.")
    public void wifiNanPublish(@RpcParameter(name = "publishData") JSONObject publishData,
            @RpcParameter(name = "publishSettings") JSONObject publishSettings,
            @RpcParameter(name = "listenerId") Integer listenerId)
                    throws RemoteException, JSONException {
        mSession = mMgr.publish(getPublishData(publishData), getPublishSettings(publishSettings),
                new NanSessionListenerPostsEvents(mNanFacadeThread.getLooper(), listenerId),
                WifiNanSessionListener.LISTEN_PUBLISH_FAIL
                        | WifiNanSessionListener.LISTEN_PUBLISH_TERMINATED
                        | WifiNanSessionListener.LISTEN_SUBSCRIBE_FAIL
                        | WifiNanSessionListener.LISTEN_SUBSCRIBE_TERMINATED
                        | WifiNanSessionListener.LISTEN_MATCH
                        | WifiNanSessionListener.LISTEN_MESSAGE_SEND_SUCCESS
                        | WifiNanSessionListener.LISTEN_MESSAGE_SEND_FAIL
                        | WifiNanSessionListener.LISTEN_MESSAGE_RECEIVED);
    }

    @Rpc(description = "Subscribe.")
    public void wifiNanSubscribe(@RpcParameter(name = "subscribeData") JSONObject subscribeData,
            @RpcParameter(name = "subscribeSettings") JSONObject subscribeSettings,
            @RpcParameter(name = "listenerId") Integer listenerId)
                    throws RemoteException, JSONException {

        mSession = mMgr.subscribe(getSubscribeData(subscribeData),
                getSubscribeSettings(subscribeSettings),
                new NanSessionListenerPostsEvents(mNanFacadeThread.getLooper(), listenerId),
                WifiNanSessionListener.LISTEN_PUBLISH_FAIL
                        | WifiNanSessionListener.LISTEN_PUBLISH_TERMINATED
                        | WifiNanSessionListener.LISTEN_SUBSCRIBE_FAIL
                        | WifiNanSessionListener.LISTEN_SUBSCRIBE_TERMINATED
                        | WifiNanSessionListener.LISTEN_MATCH
                        | WifiNanSessionListener.LISTEN_MESSAGE_SEND_SUCCESS
                        | WifiNanSessionListener.LISTEN_MESSAGE_SEND_FAIL
                        | WifiNanSessionListener.LISTEN_MESSAGE_RECEIVED);
    }

    @Rpc(description = "Send peer-to-peer NAN message")
    public void wifiNanSendMessage(
            @RpcParameter(name = "peerId", description = "The ID of the peer being communicated "
                    + "with. Obtained from a previous message or match session.") Integer peerId,
            @RpcParameter(name = "message") String message,
            @RpcParameter(name = "messageId", description = "Arbitrary handle used for "
                    + "identification of the message in the message status callbacks")
            Integer messageId)
                    throws RemoteException {
        mSession.sendMessage(peerId, message.getBytes(), message.length(), messageId);
    }

    private class NanEventListenerPostsEvents extends WifiNanEventListener {
        public NanEventListenerPostsEvents(Looper looper) {
            super(looper);
        }

        @Override
        public void onConfigCompleted(ConfigRequest configRequest) {
            Bundle mResults = new Bundle();
            mResults.putParcelable("configRequest", configRequest);
            mEventFacade.postEvent("WifiNanOnConfigCompleted", mResults);
        }

        @Override
        public void onConfigFailed(ConfigRequest failedConfig, int reason) {
            Bundle mResults = new Bundle();
            mResults.putParcelable("failedConfig", failedConfig);
            mResults.putInt("reason", reason);
            mEventFacade.postEvent("WifiNanOnConfigFailed", mResults);
        }

        @Override
        public void onNanDown(int reason) {
            Bundle mResults = new Bundle();
            mResults.putInt("reason", reason);
            mEventFacade.postEvent("WifiNanOnNanDown", mResults);
        }

        @Override
        public void onIdentityChanged() {
            Bundle mResults = new Bundle();
            mEventFacade.postEvent("WifiNanOnIdentityChanged", mResults);
        }
    }

    private class NanSessionListenerPostsEvents extends WifiNanSessionListener {
        private int mListenerId;

        public NanSessionListenerPostsEvents(Looper looper, int listenerId) {
            super(looper);
            mListenerId = listenerId;
        }

        @Override
        public void onPublishFail(int reason) {
            Bundle mResults = new Bundle();
            mResults.putInt("listenerId", mListenerId);
            mResults.putInt("reason", reason);
            mEventFacade.postEvent("WifiNanSessionOnPublishFail", mResults);
        }

        @Override
        public void onPublishTerminated(int reason) {
            Bundle mResults = new Bundle();
            mResults.putInt("listenerId", mListenerId);
            mResults.putInt("reason", reason);
            mEventFacade.postEvent("WifiNanSessionOnPublishTerminated", mResults);
        }

        @Override
        public void onSubscribeFail(int reason) {
            Bundle mResults = new Bundle();
            mResults.putInt("listenerId", mListenerId);
            mResults.putInt("reason", reason);
            mEventFacade.postEvent("WifiNanSessionOnSubscribeFail", mResults);
        }

        @Override
        public void onSubscribeTerminated(int reason) {
            Bundle mResults = new Bundle();
            mResults.putInt("listenerId", mListenerId);
            mResults.putInt("reason", reason);
            mEventFacade.postEvent("WifiNanSessionOnSubscribeTerminated", mResults);
        }

        @Override
        public void onMatch(int peerId, byte[] serviceSpecificInfo,
                int serviceSpecificInfoLength, byte[] matchFilter, int matchFilterLength) {
            Bundle mResults = new Bundle();
            mResults.putInt("listenerId", mListenerId);
            mResults.putInt("peerId", peerId);
            mResults.putInt("serviceSpecificInfoLength", serviceSpecificInfoLength);
            mResults.putByteArray("serviceSpecificInfo", serviceSpecificInfo); // TODO: base64
            mResults.putInt("matchFilterLength", matchFilterLength);
            mResults.putByteArray("matchFilter", matchFilter); // TODO: base64
            mEventFacade.postEvent("WifiNanSessionOnMatch", mResults);
        }

        @Override
        public void onMessageSendSuccess(int messageId) {
            Bundle mResults = new Bundle();
            mResults.putInt("listenerId", mListenerId);
            mResults.putInt("messageId", messageId);
            mEventFacade.postEvent("WifiNanSessionOnMessageSendSuccess", mResults);
        }

        @Override
        public void onMessageSendFail(int messageId, int reason) {
            Bundle mResults = new Bundle();
            mResults.putInt("listenerId", mListenerId);
            mResults.putInt("messageId", messageId);
            mResults.putInt("reason", reason);
            mEventFacade.postEvent("WifiNanSessionOnMessageSendFail", mResults);
        }

        @Override
        public void onMessageReceived(int peerId, byte[] message, int messageLength) {
            Bundle mResults = new Bundle();
            mResults.putInt("listenerId", mListenerId);
            mResults.putInt("peerId", peerId);
            mResults.putInt("messageLength", messageLength);
            mResults.putByteArray("message", message); // TODO: base64
            mResults.putString("messageAsString", new String(message, 0, messageLength));
            mEventFacade.postEvent("WifiNanSessionOnMessageReceived", mResults);
        }
    }
}
