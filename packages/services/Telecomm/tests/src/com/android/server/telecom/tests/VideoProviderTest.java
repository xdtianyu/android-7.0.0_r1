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

package com.android.server.telecom.tests;

import com.android.server.telecom.Log;

import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.internal.exceptions.ExceptionIncludingMockitoWarnings;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import android.content.Context;
import android.content.Intent;
import android.graphics.Camera;
import android.graphics.SurfaceTexture;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.telecom.Call;
import android.telecom.CallAudioState;
import android.telecom.Connection;
import android.telecom.Connection.VideoProvider;
import android.telecom.InCallService;
import android.telecom.InCallService.VideoCall;
import android.telecom.ParcelableCall;
import android.telecom.TelecomManager;
import android.telecom.VideoCallImpl;
import android.telecom.VideoProfile;
import android.telecom.VideoProfile.CameraCapabilities;
import android.view.Surface;

import com.google.common.base.Predicate;

import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.RunnableFuture;
import java.util.concurrent.TimeUnit;

import static android.test.MoreAsserts.assertEquals;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

/**
 * Performs tests of the {@link VideoProvider} and {@link VideoCall} APIs.  Ensures that requests
 * sent from an InCallService are routed through Telecom to a VideoProvider, and that callbacks are
 * correctly routed.
 */
public class VideoProviderTest extends TelecomSystemTest {
    private static final int ORIENTATION_0 = 0;
    private static final int ORIENTATION_90 = 90;
    private static final float ZOOM_LEVEL = 3.0f;

    @Mock private VideoCall.Callback mVideoCallCallback;
    private IdPair mCallIds;
    private InCallService.VideoCall mVideoCall;
    private VideoCallImpl mVideoCallImpl;
    private ConnectionServiceFixture.ConnectionInfo mConnectionInfo;
    private CountDownLatch mVerificationLock;

    private Answer mVerification = new Answer() {
        @Override
        public Object answer(InvocationOnMock i) {
            mVerificationLock.countDown();
            return null;
        }
    };

    @Override
    public void setUp() throws Exception {
        super.setUp();

        mCallIds = startAndMakeActiveOutgoingCall(
                "650-555-1212",
                mPhoneAccountA0.getAccountHandle(),
                mConnectionServiceFixtureA);

        // Set the video provider on the connection.
        mConnectionServiceFixtureA.sendSetVideoProvider(
                mConnectionServiceFixtureA.mLatestConnectionId);

        // Provide a mocked VideoCall.Callback to receive callbacks via.
        mVideoCallCallback = mock(InCallService.VideoCall.Callback.class);

        mVideoCall = mInCallServiceFixtureX.getCall(mCallIds.mCallId).getVideoCallImpl();
        mVideoCallImpl = (VideoCallImpl) mVideoCall;
        mVideoCall.registerCallback(mVideoCallCallback);

        mConnectionInfo = mConnectionServiceFixtureA.mConnectionById.get(mCallIds.mConnectionId);
        mVerificationLock = new CountDownLatch(1);
    }

    @Override
    public void tearDown() throws Exception {
        super.tearDown();
    }

    /**
     * Tests the {@link VideoCall#setCamera(String)}, {@link VideoProvider#onSetCamera(String)},
     * and {@link VideoCall.Callback#onCameraCapabilitiesChanged(CameraCapabilities)}
     * APIS.
     */
    public void testCameraChange() throws Exception {
        // Wait until the callback has been received before performing verification.
        doAnswer(mVerification).when(mVideoCallCallback)
                .onCameraCapabilitiesChanged(any(CameraCapabilities.class));

        // Make 2 setCamera requests.
        mVideoCall.setCamera(MockVideoProvider.CAMERA_FRONT);
        mVideoCall.setCamera(MockVideoProvider.CAMERA_BACK);

        mVerificationLock.await(TEST_TIMEOUT, TimeUnit.MILLISECONDS);

        // Capture the video profile reported via the callback.
        ArgumentCaptor<CameraCapabilities> cameraCapabilitiesCaptor =
                ArgumentCaptor.forClass(CameraCapabilities.class);

        // Verify that the callback was called twice and capture the callback arguments.
        verify(mVideoCallCallback, timeout(TEST_TIMEOUT).times(2))
                .onCameraCapabilitiesChanged(cameraCapabilitiesCaptor.capture());

        assertEquals(2, cameraCapabilitiesCaptor.getAllValues().size());

        List<CameraCapabilities> cameraCapabilities = cameraCapabilitiesCaptor.getAllValues();
        // Ensure dimensions are as expected.
        assertEquals(MockVideoProvider.CAMERA_FRONT_DIMENSIONS,
                cameraCapabilities.get(0).getHeight());
        assertEquals(MockVideoProvider.CAMERA_BACK_DIMENSIONS,
                cameraCapabilities.get(1).getHeight());
    }

    /**
     * Tests the {@link VideoCall#setPreviewSurface(Surface)} and
     * {@link VideoProvider#onSetPreviewSurface(Surface)} APIs.
     */
    public void testSetPreviewSurface() throws Exception {
        final Surface surface = new Surface(new SurfaceTexture(1));
        mVideoCall.setPreviewSurface(surface);

        assertTrueWithTimeout(new Predicate<Void>() {
            @Override
            public boolean apply(Void v) {
                return mConnectionInfo.mockVideoProvider.getPreviewSurface() == surface;
            }
        });

        mVideoCall.setPreviewSurface(null);

        assertTrueWithTimeout(new Predicate<Void>() {
            @Override
            public boolean apply(Void v) {
                return mConnectionInfo.mockVideoProvider.getPreviewSurface() == null;
            }
        });
    }

    /**
     * Tests the {@link VideoCall#setDisplaySurface(Surface)} and
     * {@link VideoProvider#onSetDisplaySurface(Surface)} APIs.
     */
    public void testSetDisplaySurface() throws Exception {
        final Surface surface = new Surface(new SurfaceTexture(1));
        mVideoCall.setDisplaySurface(surface);

        assertTrueWithTimeout(new Predicate<Void>() {
            @Override
            public boolean apply(Void v) {
                return mConnectionInfo.mockVideoProvider.getDisplaySurface() == surface;
            }
        });

        mVideoCall.setDisplaySurface(null);

        assertTrueWithTimeout(new Predicate<Void>() {
            @Override
            public boolean apply(Void v) {
                return mConnectionInfo.mockVideoProvider.getDisplaySurface() == null;
            }
        });
    }

    /**
     * Tests the {@link VideoCall#setDeviceOrientation(int)} and
     * {@link VideoProvider#onSetDeviceOrientation(int)} APIs.
     */
    public void testSetDeviceOrientation() throws Exception {
        mVideoCall.setDeviceOrientation(ORIENTATION_0);

        assertTrueWithTimeout(new Predicate<Void>() {
            @Override
            public boolean apply(Void v) {
                return mConnectionInfo.mockVideoProvider.getDeviceOrientation() == ORIENTATION_0;
            }
        });

        mVideoCall.setDeviceOrientation(ORIENTATION_90);

        assertTrueWithTimeout(new Predicate<Void>() {
            @Override
            public boolean apply(Void v) {
                return mConnectionInfo.mockVideoProvider.getDeviceOrientation() == ORIENTATION_90;
            }
        });
    }

    /**
     * Tests the {@link VideoCall#setZoom(float)} and {@link VideoProvider#onSetZoom(float)} APIs.
     */
    public void testSetZoom() throws Exception {
        mVideoCall.setZoom(ZOOM_LEVEL);

        assertTrueWithTimeout(new Predicate<Void>() {
            @Override
            public boolean apply(Void v) {
                return mConnectionInfo.mockVideoProvider.getZoom() == ZOOM_LEVEL;
            }
        });
    }

    /**
     * Tests the {@link VideoCall#sendSessionModifyRequest(VideoProfile)},
     * {@link VideoProvider#onSendSessionModifyRequest(VideoProfile, VideoProfile)},
     * {@link VideoProvider#receiveSessionModifyResponse(int, VideoProfile, VideoProfile)}, and
     * {@link VideoCall.Callback#onSessionModifyResponseReceived(int, VideoProfile, VideoProfile)}
     * APIs.
     *
     * Emulates a scenario where an InCallService sends a request to upgrade to video, which the
     * peer accepts as-is.
     */
    public void testSessionModifyRequest() throws Exception {
        VideoProfile requestProfile = new VideoProfile(VideoProfile.STATE_BIDIRECTIONAL);

        // Set the starting video state on the video call impl; normally this would be set based on
        // the original android.telecom.Call instance.
        mVideoCallImpl.setVideoState(VideoProfile.STATE_RX_ENABLED);

        doAnswer(mVerification).when(mVideoCallCallback)
                .onSessionModifyResponseReceived(anyInt(), any(VideoProfile.class),
                        any(VideoProfile.class));

        // Send the request.
        mVideoCall.sendSessionModifyRequest(requestProfile);

        mVerificationLock.await(TEST_TIMEOUT, TimeUnit.MILLISECONDS);

        // Capture the video profiles from the callback.
        ArgumentCaptor<VideoProfile> fromVideoProfileCaptor =
                ArgumentCaptor.forClass(VideoProfile.class);
        ArgumentCaptor<VideoProfile> toVideoProfileCaptor =
                ArgumentCaptor.forClass(VideoProfile.class);

        // Verify we got a response and capture the profiles.
        verify(mVideoCallCallback, timeout(TEST_TIMEOUT))
                .onSessionModifyResponseReceived(eq(VideoProvider.SESSION_MODIFY_REQUEST_SUCCESS),
                        fromVideoProfileCaptor.capture(), toVideoProfileCaptor.capture());

        assertEquals(VideoProfile.STATE_RX_ENABLED,
                fromVideoProfileCaptor.getValue().getVideoState());
        assertEquals(VideoProfile.STATE_BIDIRECTIONAL,
                toVideoProfileCaptor.getValue().getVideoState());
    }

    /**
     * Tests the {@link VideoCall#sendSessionModifyResponse(VideoProfile)},
     * and {@link VideoProvider#onSendSessionModifyResponse(VideoProfile)} APIs.
     */
    public void testSessionModifyResponse() throws Exception {
        VideoProfile sessionModifyResponse = new VideoProfile(VideoProfile.STATE_TX_ENABLED);

        mVideoCall.sendSessionModifyResponse(sessionModifyResponse);

        assertTrueWithTimeout(new Predicate<Void>() {
            @Override
            public boolean apply(Void v) {
                VideoProfile response = mConnectionInfo.mockVideoProvider
                        .getSessionModifyResponse();
                return response != null && response.getVideoState() == VideoProfile.STATE_TX_ENABLED;
            }
        });
    }

    /**
     * Tests the {@link VideoCall#requestCameraCapabilities()} ()},
     * {@link VideoProvider#onRequestCameraCapabilities()} ()}, and
     * {@link VideoCall.Callback#onCameraCapabilitiesChanged(CameraCapabilities)} APIs.
     */
    public void testRequestCameraCapabilities() throws Exception {
        // Wait until the callback has been received before performing verification.
        doAnswer(mVerification).when(mVideoCallCallback)
                .onCameraCapabilitiesChanged(any(CameraCapabilities.class));

        mVideoCall.requestCameraCapabilities();

        mVerificationLock.await(TEST_TIMEOUT, TimeUnit.MILLISECONDS);

        verify(mVideoCallCallback, timeout(TEST_TIMEOUT))
                .onCameraCapabilitiesChanged(any(CameraCapabilities.class));
    }

    /**
     * Tests the {@link VideoCall#setPauseImage(Uri)}, and
     * {@link VideoProvider#onSetPauseImage(Uri)} APIs.
     */
    public void testSetPauseImage() throws Exception {
        final Uri testUri = Uri.fromParts("file", "test.jpg", null);
        mVideoCall.setPauseImage(testUri);

        assertTrueWithTimeout(new Predicate<Void>() {
            @Override
            public boolean apply(Void v) {
                Uri pauseImage = mConnectionInfo.mockVideoProvider.getPauseImage();
                return pauseImage != null && pauseImage.equals(testUri);
            }
        });
    }

    /**
     * Tests the {@link VideoCall#requestCallDataUsage()},
     * {@link VideoProvider#onRequestConnectionDataUsage()}, and
     * {@link VideoCall.Callback#onCallDataUsageChanged(long)} APIs.
     */
    public void testRequestDataUsage() throws Exception {
        // Wait until the callback has been received before performing verification.
        doAnswer(mVerification).when(mVideoCallCallback)
                .onCallDataUsageChanged(anyLong());

        mVideoCall.requestCallDataUsage();

        mVerificationLock.await(TEST_TIMEOUT, TimeUnit.MILLISECONDS);

        verify(mVideoCallCallback, timeout(TEST_TIMEOUT))
                .onCallDataUsageChanged(eq(MockVideoProvider.DATA_USAGE));
    }

    /**
     * Tests the {@link VideoProvider#receiveSessionModifyRequest(VideoProfile)},
     * {@link VideoCall.Callback#onSessionModifyRequestReceived(VideoProfile)} APIs.
     */
    public void testReceiveSessionModifyRequest() throws Exception {
        // Wait until the callback has been received before performing verification.
        doAnswer(mVerification).when(mVideoCallCallback)
                .onSessionModifyRequestReceived(any(VideoProfile.class));

        mConnectionInfo.mockVideoProvider.sendMockSessionModifyRequest();

        mVerificationLock.await(TEST_TIMEOUT, TimeUnit.MILLISECONDS);

        ArgumentCaptor<VideoProfile> requestProfileCaptor =
                ArgumentCaptor.forClass(VideoProfile.class);
        verify(mVideoCallCallback, timeout(TEST_TIMEOUT))
                .onSessionModifyRequestReceived(requestProfileCaptor.capture());
        assertEquals(VideoProfile.STATE_BIDIRECTIONAL,
                requestProfileCaptor.getValue().getVideoState());
    }


    /**
     * Tests the {@link VideoProvider#handleCallSessionEvent(int)}, and
     * {@link VideoCall.Callback#onCallSessionEvent(int)} APIs.
     */
    public void testSessionEvent() throws Exception {
        // Wait until the callback has been received before performing verification.
        doAnswer(mVerification).when(mVideoCallCallback)
                .onCallSessionEvent(anyInt());

        mConnectionInfo.mockVideoProvider.sendMockSessionEvent(
                VideoProvider.SESSION_EVENT_CAMERA_READY);

        mVerificationLock.await(TEST_TIMEOUT, TimeUnit.MILLISECONDS);

        verify(mVideoCallCallback, timeout(TEST_TIMEOUT))
                .onCallSessionEvent(eq(VideoProvider.SESSION_EVENT_CAMERA_READY));
    }

    /**
     * Tests the {@link VideoProvider#changePeerDimensions(int, int)} and
     * {@link VideoCall.Callback#onPeerDimensionsChanged(int, int)} APIs.
     */
    public void testPeerDimensionChange() throws Exception {
        // Wait until the callback has been received before performing verification.
        doAnswer(mVerification).when(mVideoCallCallback)
                .onPeerDimensionsChanged(anyInt(), anyInt());

        mConnectionInfo.mockVideoProvider.sendMockPeerDimensions(MockVideoProvider.PEER_DIMENSIONS,
                MockVideoProvider.PEER_DIMENSIONS);

        mVerificationLock.await(TEST_TIMEOUT, TimeUnit.MILLISECONDS);

        verify(mVideoCallCallback, timeout(TEST_TIMEOUT))
                .onPeerDimensionsChanged(eq(MockVideoProvider.PEER_DIMENSIONS),
                        eq(MockVideoProvider.PEER_DIMENSIONS));
    }

    /**
     * Tests the {@link VideoProvider#changeVideoQuality(int)} and
     * {@link VideoCall.Callback#onVideoQualityChanged(int)} APIs.
     */
    public void testVideoQualityChange() throws Exception {
        // Wait until the callback has been received before performing verification.
        doAnswer(mVerification).when(mVideoCallCallback)
                .onVideoQualityChanged(anyInt());

        mConnectionInfo.mockVideoProvider.sendMockVideoQuality(VideoProfile.QUALITY_HIGH);

        mVerificationLock.await(TEST_TIMEOUT, TimeUnit.MILLISECONDS);

        verify(mVideoCallCallback, timeout(TEST_TIMEOUT))
                .onVideoQualityChanged(eq(VideoProfile.QUALITY_HIGH));
    }
}
