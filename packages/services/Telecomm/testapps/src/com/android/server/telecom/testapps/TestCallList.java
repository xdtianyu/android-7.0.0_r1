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

package com.android.server.telecom.testapps;

import android.telecom.Call;
import android.telecom.InCallService;
import android.telecom.VideoProfile;
import android.telecom.VideoProfile.CameraCapabilities;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.Log;

import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Maintains a list of calls received via the {@link TestInCallServiceImpl}.
 */
public class TestCallList extends Call.Listener {

    public static abstract class Listener {
        public void onCallAdded(Call call) {}
        public void onCallRemoved(Call call) {}
    }

    private static final TestCallList INSTANCE = new TestCallList();
    private static final String TAG = "TestCallList";

    private class TestVideoCallListener extends InCallService.VideoCall.Callback {
        private Call mCall;

        public TestVideoCallListener(Call call) {
            mCall = call;
        }

        @Override
        public void onSessionModifyRequestReceived(VideoProfile videoProfile) {
            Log.v(TAG,
                    "onSessionModifyRequestReceived: videoState = " + videoProfile.getVideoState()
                            + " call = " + mCall);
        }

        @Override
        public void onSessionModifyResponseReceived(int status, VideoProfile requestedProfile,
                VideoProfile responseProfile) {
            Log.v(TAG,
                    "onSessionModifyResponseReceived: status = " + status + " videoState = "
                            + responseProfile.getVideoState()
                            + " call = " + mCall);
        }

        @Override
        public void onCallSessionEvent(int event) {

        }

        @Override
        public void onPeerDimensionsChanged(int width, int height) {

        }

        @Override
        public void onVideoQualityChanged(int videoQuality) {
            Log.v(TAG,
                    "onVideoQualityChanged: videoQuality = " + videoQuality + " call = " + mCall);
        }

        @Override
        public void onCallDataUsageChanged(long dataUsage) {

        }

        @Override
        public void onCameraCapabilitiesChanged(CameraCapabilities cameraCapabilities) {

        }
    }

    // The calls the call list knows about.
    private List<Call> mCalls = new LinkedList<Call>();
    private Map<Call, TestVideoCallListener> mVideoCallListeners =
            new ArrayMap<Call, TestVideoCallListener>();
    private Set<Listener> mListeners = new ArraySet<Listener>();

    /**
     * Singleton accessor.
     */
    public static TestCallList getInstance() {
        return INSTANCE;
    }

    public void addListener(Listener listener) {
        if (listener != null) {
            mListeners.add(listener);
        }
    }

    public boolean removeListener(Listener listener) {
        return mListeners.remove(listener);
    }

    public Call getCall(int position) {
        return mCalls.get(position);
    }

    public void addCall(Call call) {
        if (mCalls.contains(call)) {
            Log.e(TAG, "addCall: Call already added.");
            return;
        }
        Log.i(TAG, "addCall: " + call + " " + System.identityHashCode(this));
        mCalls.add(call);
        call.addListener(this);

        for (Listener l : mListeners) {
            l.onCallAdded(call);
        }
    }

    public void removeCall(Call call) {
        if (!mCalls.contains(call)) {
            Log.e(TAG, "removeCall: Call cannot be removed -- doesn't exist.");
            return;
        }
        Log.i(TAG, "removeCall: " + call);
        mCalls.remove(call);
        call.removeListener(this);

        for (Listener l : mListeners) {
            l.onCallRemoved(call);
        }
    }

    public void clearCalls() {
        for (Call call : new LinkedList<Call>(mCalls)) {
            removeCall(call);
        }

        for (Call call : mVideoCallListeners.keySet()) {
            if (call.getVideoCall() != null) {
                call.getVideoCall().destroy();
            }
        }
        mVideoCallListeners.clear();
    }

    public int size() {
        return mCalls.size();
    }

    /**
     * For any video calls tracked, sends an upgrade to video request.
     */
    public void sendUpgradeToVideoRequest(int videoState) {
        Log.v(TAG, "sendUpgradeToVideoRequest : videoState = " + videoState);

        for (Call call : mCalls) {
            InCallService.VideoCall videoCall = call.getVideoCall();
            Log.v(TAG, "sendUpgradeToVideoRequest: checkCall "+call);
            if (videoCall == null) {
                continue;
            }

            Log.v(TAG, "send upgrade to video request for call: " + call);
            videoCall.sendSessionModifyRequest(new VideoProfile(videoState));
        }
    }

    /**
     * For any video calls which are active, sends an upgrade to video response with the specified
     * video state.
     *
     * @param videoState The video state to respond with.
     */
    public void sendUpgradeToVideoResponse(int videoState) {
        Log.v(TAG, "sendUpgradeToVideoResponse : videoState = " + videoState);

        for (Call call : mCalls) {
            InCallService.VideoCall videoCall = call.getVideoCall();
            if (videoCall == null) {
                continue;
            }

            Log.v(TAG, "send upgrade to video response for call: " + call);
            videoCall.sendSessionModifyResponse(new VideoProfile(videoState));
        }
    }

    @Override
    public void onVideoCallChanged(Call call, InCallService.VideoCall videoCall) {
        Log.v(TAG, "onVideoCallChanged: call = " + call + " " + System.identityHashCode(this));
        if (videoCall != null) {
            if (!mVideoCallListeners.containsKey(call)) {
                TestVideoCallListener listener = new TestVideoCallListener(call);
                videoCall.registerCallback(listener);
                mVideoCallListeners.put(call, listener);
                Log.v(TAG, "onVideoCallChanged: added new listener");
            }
        }
    }
}
