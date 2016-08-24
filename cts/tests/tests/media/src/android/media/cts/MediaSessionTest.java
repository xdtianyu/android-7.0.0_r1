/*
 * Copyright (C) 2014 The Android Open Source Project
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
package android.media.cts;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.media.MediaDescription;
import android.media.MediaMetadata;
import android.media.Rating;
import android.media.VolumeProvider;
import android.media.session.MediaController;
import android.media.session.MediaSession;
import android.media.session.MediaSession.QueueItem;
import android.media.session.PlaybackState;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Parcel;
import android.test.AndroidTestCase;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

public class MediaSessionTest extends AndroidTestCase {
    // The maximum time to wait for an operation.
    private static final long TIME_OUT_MS = 3000L;
    private static final int MAX_AUDIO_INFO_CHANGED_CALLBACK_COUNT = 10;
    private static final String TEST_SESSION_TAG = "test-session-tag";
    private static final String TEST_KEY = "test-key";
    private static final String TEST_VALUE = "test-val";
    private static final String TEST_SESSION_EVENT = "test-session-event";
    private static final int TEST_CURRENT_VOLUME = 10;
    private static final int TEST_MAX_VOLUME = 11;
    private static final long TEST_QUEUE_ID = 12L;
    private static final long TEST_ACTION = 55L;

    private AudioManager mAudioManager;
    private Handler mHandler = new Handler(Looper.getMainLooper());
    private Object mWaitLock = new Object();
    private MediaControllerCallback mCallback = new MediaControllerCallback();
    private MediaSession mSession;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mAudioManager = (AudioManager) getContext().getSystemService(Context.AUDIO_SERVICE);
        mSession = new MediaSession(getContext(), TEST_SESSION_TAG);
    }

    @Override
    protected void tearDown() throws Exception {
        // It is OK to call release() twice.
        mSession.release();
        super.tearDown();
    }

    /**
     * Tests that a session can be created and that all the fields are
     * initialized correctly.
     */
    public void testCreateSession() throws Exception {
        assertNotNull(mSession.getSessionToken());
        assertFalse("New session should not be active", mSession.isActive());

        // Verify by getting the controller and checking all its fields
        MediaController controller = mSession.getController();
        assertNotNull(controller);
        verifyNewSession(controller, TEST_SESSION_TAG);
    }

    /**
     * Tests MediaSession.Token created in the constructor of MediaSession.
     */
    public void testSessionToken() throws Exception {
        MediaSession.Token sessionToken = mSession.getSessionToken();

        assertNotNull(sessionToken);
        assertEquals(0, sessionToken.describeContents());

        // Test writeToParcel
        Parcel p = Parcel.obtain();
        sessionToken.writeToParcel(p, 0);
        p.setDataPosition(0);
        MediaSession.Token token = MediaSession.Token.CREATOR.createFromParcel(p);
        assertEquals(token, sessionToken);
        p.recycle();
    }

    /**
     * Tests that the various configuration bits on a session get passed to the
     * controller.
     */
    public void testConfigureSession() throws Exception {
        MediaController controller = mSession.getController();
        controller.registerCallback(mCallback, mHandler);

        synchronized (mWaitLock) {
            // test setExtras
            mCallback.resetLocked();
            final Bundle extras = new Bundle();
            extras.putString(TEST_KEY, TEST_VALUE);
            mSession.setExtras(extras);
            mWaitLock.wait(TIME_OUT_MS);
            assertTrue(mCallback.mOnExtraChangedCalled);

            Bundle extrasOut = mCallback.mExtras;
            assertNotNull(extrasOut);
            assertEquals(TEST_VALUE, extrasOut.get(TEST_KEY));

            extrasOut = controller.getExtras();
            assertNotNull(extrasOut);
            assertEquals(TEST_VALUE, extrasOut.get(TEST_KEY));

            // test setFlags
            mSession.setFlags(5);
            assertEquals(5, controller.getFlags());

            // test setMetadata
            mCallback.resetLocked();
            MediaMetadata metadata =
                    new MediaMetadata.Builder().putString(TEST_KEY, TEST_VALUE).build();
            mSession.setMetadata(metadata);
            mWaitLock.wait(TIME_OUT_MS);
            assertTrue(mCallback.mOnMetadataChangedCalled);

            MediaMetadata metadataOut = mCallback.mMediaMetadata;
            assertNotNull(metadataOut);
            assertEquals(TEST_VALUE, metadataOut.getString(TEST_KEY));

            metadataOut = controller.getMetadata();
            assertNotNull(metadataOut);
            assertEquals(TEST_VALUE, metadataOut.getString(TEST_KEY));

            // test setPlaybackState
            mCallback.resetLocked();
            PlaybackState state = new PlaybackState.Builder().setActions(TEST_ACTION).build();
            mSession.setPlaybackState(state);
            mWaitLock.wait(TIME_OUT_MS);
            assertTrue(mCallback.mOnPlaybackStateChangedCalled);

            PlaybackState stateOut = mCallback.mPlaybackState;
            assertNotNull(stateOut);
            assertEquals(TEST_ACTION, stateOut.getActions());

            stateOut = controller.getPlaybackState();
            assertNotNull(stateOut);
            assertEquals(TEST_ACTION, stateOut.getActions());

            // test setQueue and setQueueTitle
            mCallback.resetLocked();
            List<QueueItem> queue = new ArrayList<>();
            QueueItem item = new QueueItem(new MediaDescription.Builder()
                    .setMediaId(TEST_VALUE).setTitle("title").build(), TEST_QUEUE_ID);
            queue.add(item);
            mSession.setQueue(queue);
            mWaitLock.wait(TIME_OUT_MS);
            assertTrue(mCallback.mOnQueueChangedCalled);

            mSession.setQueueTitle(TEST_VALUE);
            mWaitLock.wait(TIME_OUT_MS);
            assertTrue(mCallback.mOnQueueTitleChangedCalled);

            assertEquals(TEST_VALUE, mCallback.mTitle);
            assertEquals(queue.size(), mCallback.mQueue.size());
            assertEquals(TEST_QUEUE_ID, mCallback.mQueue.get(0).getQueueId());
            assertEquals(TEST_VALUE, mCallback.mQueue.get(0).getDescription().getMediaId());

            assertEquals(TEST_VALUE, controller.getQueueTitle());
            assertEquals(queue.size(), controller.getQueue().size());
            assertEquals(TEST_QUEUE_ID, controller.getQueue().get(0).getQueueId());
            assertEquals(TEST_VALUE, controller.getQueue().get(0).getDescription().getMediaId());

            mCallback.resetLocked();
            mSession.setQueue(null);
            mWaitLock.wait(TIME_OUT_MS);
            assertTrue(mCallback.mOnQueueChangedCalled);

            mSession.setQueueTitle(null);
            mWaitLock.wait(TIME_OUT_MS);
            assertTrue(mCallback.mOnQueueTitleChangedCalled);

            assertNull(mCallback.mTitle);
            assertNull(mCallback.mQueue);
            assertNull(controller.getQueueTitle());
            assertNull(controller.getQueue());

            // test setSessionActivity
            Intent intent = new Intent("cts.MEDIA_SESSION_ACTION");
            PendingIntent pi = PendingIntent.getActivity(getContext(), 555, intent, 0);
            mSession.setSessionActivity(pi);
            assertEquals(pi, controller.getSessionActivity());

            // test setActivity
            mSession.setActive(true);
            assertTrue(mSession.isActive());

            // test sendSessionEvent
            mCallback.resetLocked();
            mSession.sendSessionEvent(TEST_SESSION_EVENT, extras);
            mWaitLock.wait(TIME_OUT_MS);

            assertTrue(mCallback.mOnSessionEventCalled);
            assertEquals(TEST_SESSION_EVENT, mCallback.mEvent);
            assertEquals(TEST_VALUE, mCallback.mExtras.getString(TEST_KEY));

            // test release
            mCallback.resetLocked();
            mSession.release();
            mWaitLock.wait(TIME_OUT_MS);
            assertTrue(mCallback.mOnSessionDestroyedCalled);
        }
    }

    /**
     * Tests for setPlaybackToLocal and setPlaybackToRemote.
     */
    public void testPlaybackToLocalAndRemote() throws Exception {
        MediaController controller = mSession.getController();
        controller.registerCallback(mCallback, mHandler);

        synchronized (mWaitLock) {
            // test setPlaybackToRemote, do this before testing setPlaybackToLocal
            // to ensure it switches correctly.
            mCallback.resetLocked();
            try {
                mSession.setPlaybackToRemote(null);
                fail("Expected IAE for setPlaybackToRemote(null)");
            } catch (IllegalArgumentException e) {
                // expected
            }
            VolumeProvider vp = new VolumeProvider(VolumeProvider.VOLUME_CONTROL_FIXED,
                    TEST_MAX_VOLUME, TEST_CURRENT_VOLUME) {};
            mSession.setPlaybackToRemote(vp);

            MediaController.PlaybackInfo info = null;
            for (int i = 0; i < MAX_AUDIO_INFO_CHANGED_CALLBACK_COUNT; ++i) {
                mCallback.mOnAudioInfoChangedCalled = false;
                mWaitLock.wait(TIME_OUT_MS);
                assertTrue(mCallback.mOnAudioInfoChangedCalled);
                info = mCallback.mPlaybackInfo;
                if (info != null && info.getCurrentVolume() == TEST_CURRENT_VOLUME
                        && info.getMaxVolume() == TEST_MAX_VOLUME
                        && info.getVolumeControl() == VolumeProvider.VOLUME_CONTROL_FIXED
                        && info.getPlaybackType()
                                == MediaController.PlaybackInfo.PLAYBACK_TYPE_REMOTE) {
                    break;
                }
            }
            assertNotNull(info);
            assertEquals(MediaController.PlaybackInfo.PLAYBACK_TYPE_REMOTE, info.getPlaybackType());
            assertEquals(TEST_MAX_VOLUME, info.getMaxVolume());
            assertEquals(TEST_CURRENT_VOLUME, info.getCurrentVolume());
            assertEquals(VolumeProvider.VOLUME_CONTROL_FIXED, info.getVolumeControl());

            info = controller.getPlaybackInfo();
            assertNotNull(info);
            assertEquals(MediaController.PlaybackInfo.PLAYBACK_TYPE_REMOTE, info.getPlaybackType());
            assertEquals(TEST_MAX_VOLUME, info.getMaxVolume());
            assertEquals(TEST_CURRENT_VOLUME, info.getCurrentVolume());
            assertEquals(VolumeProvider.VOLUME_CONTROL_FIXED, info.getVolumeControl());

            // test setPlaybackToLocal
            AudioAttributes attrs = new AudioAttributes.Builder().addTag(TEST_VALUE).build();
            mSession.setPlaybackToLocal(attrs);

            info = controller.getPlaybackInfo();
            assertNotNull(info);
            assertEquals(MediaController.PlaybackInfo.PLAYBACK_TYPE_LOCAL, info.getPlaybackType());
            Set<String> tags = info.getAudioAttributes().getTags();
            assertNotNull(tags);
            assertTrue(tags.contains(TEST_VALUE));
        }
    }

    /**
     * Tests MediaSession.QueueItem.
     */
    public void testQueueItem() {
        QueueItem item = new QueueItem(new MediaDescription.Builder()
                .setMediaId("media-id").setTitle("title").build(), TEST_QUEUE_ID);
        assertEquals(TEST_QUEUE_ID, item.getQueueId());
        assertEquals("media-id", item.getDescription().getMediaId());
        assertEquals("title", item.getDescription().getTitle());

        Parcel p = Parcel.obtain();
        item.writeToParcel(p, 0);
        p.setDataPosition(0);
        QueueItem other = QueueItem.CREATOR.createFromParcel(p);
        assertEquals(item.toString(), other.toString());
        p.recycle();
    }

    /**
     * Verifies that a new session hasn't had any configuration bits set yet.
     *
     * @param controller The controller for the session
     */
    private void verifyNewSession(MediaController controller, String tag) {
        assertEquals("New session has unexpected configuration", 0L, controller.getFlags());
        assertNull("New session has unexpected configuration", controller.getExtras());
        assertNull("New session has unexpected configuration", controller.getMetadata());
        assertEquals("New session has unexpected configuration",
                getContext().getPackageName(), controller.getPackageName());
        assertNull("New session has unexpected configuration", controller.getPlaybackState());
        assertNull("New session has unexpected configuration", controller.getQueue());
        assertNull("New session has unexpected configuration", controller.getQueueTitle());
        assertEquals("New session has unexpected configuration", Rating.RATING_NONE,
                controller.getRatingType());
        assertNull("New session has unexpected configuration", controller.getSessionActivity());

        assertNotNull(controller.getSessionToken());
        assertNotNull(controller.getTransportControls());
        assertEquals(tag, controller.getTag());

        MediaController.PlaybackInfo info = controller.getPlaybackInfo();
        assertNotNull(info);
        assertEquals(MediaController.PlaybackInfo.PLAYBACK_TYPE_LOCAL, info.getPlaybackType());
        AudioAttributes attrs = info.getAudioAttributes();
        assertNotNull(attrs);
        assertEquals(AudioAttributes.USAGE_MEDIA, attrs.getUsage());
        assertEquals(mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC),
                info.getCurrentVolume());
    }

    private class MediaControllerCallback extends MediaController.Callback {
        private volatile boolean mOnPlaybackStateChangedCalled;
        private volatile boolean mOnMetadataChangedCalled;
        private volatile boolean mOnQueueChangedCalled;
        private volatile boolean mOnQueueTitleChangedCalled;
        private volatile boolean mOnExtraChangedCalled;
        private volatile boolean mOnAudioInfoChangedCalled;
        private volatile boolean mOnSessionDestroyedCalled;
        private volatile boolean mOnSessionEventCalled;

        private volatile PlaybackState mPlaybackState;
        private volatile MediaMetadata mMediaMetadata;
        private volatile List<QueueItem> mQueue;
        private volatile CharSequence mTitle;
        private volatile String mEvent;
        private volatile Bundle mExtras;
        private volatile MediaController.PlaybackInfo mPlaybackInfo;

        public void resetLocked() {
            mOnPlaybackStateChangedCalled = false;
            mOnMetadataChangedCalled = false;
            mOnQueueChangedCalled = false;
            mOnQueueTitleChangedCalled = false;
            mOnExtraChangedCalled = false;
            mOnAudioInfoChangedCalled = false;
            mOnSessionDestroyedCalled = false;
            mOnSessionEventCalled = false;

            mPlaybackState = null;
            mMediaMetadata = null;
            mQueue = null;
            mTitle = null;
            mExtras = null;
            mPlaybackInfo = null;
        }

        @Override
        public void onPlaybackStateChanged(PlaybackState state) {
            synchronized (mWaitLock) {
                mOnPlaybackStateChangedCalled = true;
                mPlaybackState = state;
                mWaitLock.notify();
            }
        }

        @Override
        public void onMetadataChanged(MediaMetadata metadata) {
            synchronized (mWaitLock) {
                mOnMetadataChangedCalled = true;
                mMediaMetadata = metadata;
                mWaitLock.notify();
            }
        }

        @Override
        public void onQueueChanged(List<QueueItem> queue) {
            synchronized (mWaitLock) {
                mOnQueueChangedCalled = true;
                mQueue = queue;
                mWaitLock.notify();
            }
        }

        @Override
        public void onQueueTitleChanged(CharSequence title) {
            synchronized (mWaitLock) {
                mOnQueueTitleChangedCalled = true;
                mTitle = title;
                mWaitLock.notify();
            }
        }

        @Override
        public void onExtrasChanged(Bundle extras) {
            synchronized (mWaitLock) {
                mOnExtraChangedCalled = true;
                mExtras = extras;
                mWaitLock.notify();
            }
        }

        @Override
        public void onAudioInfoChanged(MediaController.PlaybackInfo info) {
            synchronized (mWaitLock) {
                mOnAudioInfoChangedCalled = true;
                mPlaybackInfo = info;
                mWaitLock.notify();
            }
        }

        @Override
        public void onSessionDestroyed() {
            synchronized (mWaitLock) {
                mOnSessionDestroyedCalled = true;
                mWaitLock.notify();
            }
        }

        @Override
        public void onSessionEvent(String event, Bundle extras) {
            synchronized (mWaitLock) {
                mOnSessionEventCalled = true;
                mEvent = event;
                mExtras = (Bundle) extras.clone();
                mWaitLock.notify();
            }
        }
    }
}
