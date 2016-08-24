/*
 * Copyright (C) 2011 The Android Open Source Project
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

import android.cts.util.MediaUtils;
import android.media.MediaFormat;
import android.media.MediaPlayer;
import android.media.MediaPlayer.TrackInfo;
import android.media.TimedMetaData;
import android.os.Looper;
import android.os.PowerManager;
import android.os.SystemClock;
import android.util.Log;
import android.webkit.cts.CtsTestServer;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Tests of MediaPlayer streaming capabilities.
 */
public class StreamingMediaPlayerTest extends MediaPlayerTestBase {
    private static final String TAG = "StreamingMediaPlayerTest";

    private CtsTestServer mServer;

/* RTSP tests are more flaky and vulnerable to network condition.
   Disable until better solution is available
    // Streaming RTSP video from YouTube
    public void testRTSP_H263_AMR_Video1() throws Exception {
        playVideoTest("rtsp://v2.cache7.c.youtube.com/video.3gp?cid=0x271de9756065677e"
                + "&fmt=13&user=android-device-test", 176, 144);
    }
    public void testRTSP_H263_AMR_Video2() throws Exception {
        playVideoTest("rtsp://v2.cache7.c.youtube.com/video.3gp?cid=0xc80658495af60617"
                + "&fmt=13&user=android-device-test", 176, 144);
    }

    public void testRTSP_MPEG4SP_AAC_Video1() throws Exception {
        playVideoTest("rtsp://v2.cache7.c.youtube.com/video.3gp?cid=0x271de9756065677e"
                + "&fmt=17&user=android-device-test", 176, 144);
    }
    public void testRTSP_MPEG4SP_AAC_Video2() throws Exception {
        playVideoTest("rtsp://v2.cache7.c.youtube.com/video.3gp?cid=0xc80658495af60617"
                + "&fmt=17&user=android-device-test", 176, 144);
    }

    public void testRTSP_H264Base_AAC_Video1() throws Exception {
        playVideoTest("rtsp://v2.cache7.c.youtube.com/video.3gp?cid=0x271de9756065677e"
                + "&fmt=18&user=android-device-test", 480, 270);
    }
    public void testRTSP_H264Base_AAC_Video2() throws Exception {
        playVideoTest("rtsp://v2.cache7.c.youtube.com/video.3gp?cid=0xc80658495af60617"
                + "&fmt=18&user=android-device-test", 480, 270);
    }
*/
    // Streaming HTTP video from YouTube
    public void testHTTP_H263_AMR_Video1() throws Exception {
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_H263, MediaFormat.MIMETYPE_AUDIO_AMR_NB)) {
            return; // skip
        }

        playVideoTest("http://redirector.c.youtube.com/videoplayback?id=271de9756065677e"
                + "&itag=13&source=youtube&ip=0.0.0.0&ipbits=0&expire=19000000000"
                + "&sparams=ip,ipbits,expire,id,itag,source"
                + "&signature=5729247E22691EBB3E804DDD523EC42DC17DD8CE"
                + ".443B81C1E8E6D64E4E1555F568BA46C206507D78"
                + "&key=ik0&user=android-device-test", 176, 144);
    }

    public void testHTTP_H263_AMR_Video2() throws Exception {
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_H263, MediaFormat.MIMETYPE_AUDIO_AMR_NB)) {
            return; // skip
        }

        playVideoTest("http://redirector.c.youtube.com/videoplayback?id=c80658495af60617"
                + "&itag=13&source=youtube&ip=0.0.0.0&ipbits=0&expire=19000000000"
                + "&sparams=ip,ipbits,expire,id,itag,source"
                + "&signature=508D82AB36939345BF6B8D0623CB6CABDD9C64C3"
                + ".9B3336A96846DF38E5343C46AA57F6CF2956E427"
                + "&key=ik0&user=android-device-test", 176, 144);
    }

    public void testHTTP_MPEG4SP_AAC_Video1() throws Exception {
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_MPEG4)) {
            return; // skip
        }

        playVideoTest("http://redirector.c.youtube.com/videoplayback?id=271de9756065677e"
                + "&itag=17&source=youtube&ip=0.0.0.0&ipbits=0&expire=19000000000"
                + "&sparams=ip,ipbits,expire,id,itag,source"
                + "&signature=837198AAADF6F36BA6B2D324F690A7C5B7AFE3FF"
                + ".7138CE5E36D718220726C1FC305497FF2D082249"
                + "&key=ik0&user=android-device-test", 176, 144);
    }

    public void testHTTP_MPEG4SP_AAC_Video2() throws Exception {
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_MPEG4)) {
            return; // skip
        }

        playVideoTest("http://redirector.c.youtube.com/videoplayback?id=c80658495af60617"
                + "&itag=17&source=youtube&ip=0.0.0.0&ipbits=0&expire=19000000000"
                + "&sparams=ip,ipbits,expire,id,itag,source"
                + "&signature=70E979A621001201BC18622BDBF914FA870BDA40"
                + ".6E78890B80F4A33A18835F775B1FF64F0A4D0003"
                + "&key=ik0&user=android-device-test", 176, 144);
    }

    public void testHTTP_H264Base_AAC_Video1() throws Exception {
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }

        playVideoTest("http://redirector.c.youtube.com/videoplayback?id=271de9756065677e"
                + "&itag=18&source=youtube&ip=0.0.0.0&ipbits=0&expire=19000000000"
                + "&sparams=ip,ipbits,expire,id,itag,source"
                + "&signature=667AEEF54639926662CE62361400B8F8C1753B3F"
                + ".15F46C382C68A9F121BA17BF1F56BEDEB4B06091"
                + "&key=ik0&user=android-device-test", 640, 360);
    }

    public void testHTTP_H264Base_AAC_Video2() throws Exception {
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }

        playVideoTest("http://redirector.c.youtube.com/videoplayback?id=c80658495af60617"
                + "&itag=18&source=youtube&ip=0.0.0.0&ipbits=0&expire=19000000000"
                + "&sparams=ip,ipbits,expire,id,itag,source"
                + "&signature=46A04ED550CA83B79B60060BA80C79FDA5853D26"
                + ".49582D382B4A9AFAA163DED38D2AE531D85603C0"
                + "&key=ik0&user=android-device-test", 640, 360);
    }

    // Streaming HLS video from YouTube
    public void testHLS() throws Exception {
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }

        // Play stream for 60 seconds
        playLiveVideoTest("http://www.youtube.com/api/manifest/hls_variant/id/"
                + "0168724d02bd9945/itag/5/source/youtube/playlist_type/DVR/ip/"
                + "0.0.0.0/ipbits/0/expire/19000000000/sparams/ip,ipbits,expire"
                + ",id,itag,source,playlist_type/signature/773AB8ACC68A96E5AA48"
                + "1996AD6A1BBCB70DCB87.95733B544ACC5F01A1223A837D2CF04DF85A336"
                + "0/key/ik0/file/m3u8", 60 * 1000);
    }

    // Streaming audio from local HTTP server
    public void testPlayMp3Stream1() throws Throwable {
        localHttpAudioStreamTest("ringer.mp3", false, false);
    }
    public void testPlayMp3Stream2() throws Throwable {
        localHttpAudioStreamTest("ringer.mp3", false, false);
    }
    public void testPlayMp3StreamRedirect() throws Throwable {
        localHttpAudioStreamTest("ringer.mp3", true, false);
    }
    public void testPlayMp3StreamNoLength() throws Throwable {
        localHttpAudioStreamTest("noiseandchirps.mp3", false, true);
    }
    public void testPlayOggStream() throws Throwable {
        localHttpAudioStreamTest("noiseandchirps.ogg", false, false);
    }
    public void testPlayOggStreamRedirect() throws Throwable {
        localHttpAudioStreamTest("noiseandchirps.ogg", true, false);
    }
    public void testPlayOggStreamNoLength() throws Throwable {
        localHttpAudioStreamTest("noiseandchirps.ogg", false, true);
    }
    public void testPlayMp3Stream1Ssl() throws Throwable {
        localHttpsAudioStreamTest("ringer.mp3", false, false);
    }

    private void localHttpAudioStreamTest(final String name, boolean redirect, boolean nolength)
            throws Throwable {
        mServer = new CtsTestServer(mContext);
        try {
            String stream_url = null;
            if (redirect) {
                // Stagefright doesn't have a limit, but we can't test support of infinite redirects
                // Up to 4 redirects seems reasonable though.
                stream_url = mServer.getRedirectingAssetUrl(name, 4);
            } else {
                stream_url = mServer.getAssetUrl(name);
            }
            if (nolength) {
                stream_url = stream_url + "?" + CtsTestServer.NOLENGTH_POSTFIX;
            }

            if (!MediaUtils.checkCodecsForPath(mContext, stream_url)) {
                return; // skip
            }

            mMediaPlayer.setDataSource(stream_url);

            mMediaPlayer.setDisplay(getActivity().getSurfaceHolder());
            mMediaPlayer.setScreenOnWhilePlaying(true);

            mOnBufferingUpdateCalled.reset();
            mMediaPlayer.setOnBufferingUpdateListener(new MediaPlayer.OnBufferingUpdateListener() {
                @Override
                public void onBufferingUpdate(MediaPlayer mp, int percent) {
                    mOnBufferingUpdateCalled.signal();
                }
            });
            mMediaPlayer.setOnErrorListener(new MediaPlayer.OnErrorListener() {
                @Override
                public boolean onError(MediaPlayer mp, int what, int extra) {
                    fail("Media player had error " + what + " playing " + name);
                    return true;
                }
            });

            assertFalse(mOnBufferingUpdateCalled.isSignalled());
            mMediaPlayer.prepare();

            if (nolength) {
                mMediaPlayer.start();
                Thread.sleep(LONG_SLEEP_TIME);
                assertFalse(mMediaPlayer.isPlaying());
            } else {
                mOnBufferingUpdateCalled.waitForSignal();
                mMediaPlayer.start();
                Thread.sleep(SLEEP_TIME);
            }
            mMediaPlayer.stop();
            mMediaPlayer.reset();
        } finally {
            mServer.shutdown();
        }
    }
    private void localHttpsAudioStreamTest(final String name, boolean redirect, boolean nolength)
            throws Throwable {
        mServer = new CtsTestServer(mContext, true);
        try {
            String stream_url = null;
            if (redirect) {
                // Stagefright doesn't have a limit, but we can't test support of infinite redirects
                // Up to 4 redirects seems reasonable though.
                stream_url = mServer.getRedirectingAssetUrl(name, 4);
            } else {
                stream_url = mServer.getAssetUrl(name);
            }
            if (nolength) {
                stream_url = stream_url + "?" + CtsTestServer.NOLENGTH_POSTFIX;
            }

            mMediaPlayer.setDataSource(stream_url);

            mMediaPlayer.setDisplay(getActivity().getSurfaceHolder());
            mMediaPlayer.setScreenOnWhilePlaying(true);

            mOnBufferingUpdateCalled.reset();
            mMediaPlayer.setOnBufferingUpdateListener(new MediaPlayer.OnBufferingUpdateListener() {
                @Override
                public void onBufferingUpdate(MediaPlayer mp, int percent) {
                    mOnBufferingUpdateCalled.signal();
                }
            });
            mMediaPlayer.setOnErrorListener(new MediaPlayer.OnErrorListener() {
                @Override
                public boolean onError(MediaPlayer mp, int what, int extra) {
                    fail("Media player had error " + what + " playing " + name);
                    return true;
                }
            });

            assertFalse(mOnBufferingUpdateCalled.isSignalled());
            try {
                mMediaPlayer.prepare();
            } catch (Exception ex) {
                return;
            }
            fail("https playback should have failed");
        } finally {
            mServer.shutdown();
        }
    }

    public void testPlayHlsStream() throws Throwable {
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }
        localHlsTest("hls.m3u8", false, false);
    }

    public void testPlayHlsStreamWithQueryString() throws Throwable {
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }
        localHlsTest("hls.m3u8", true, false);
    }

    public void testPlayHlsStreamWithRedirect() throws Throwable {
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }
        localHlsTest("hls.m3u8", false, true);
    }

    public void testPlayHlsStreamWithTimedId3() throws Throwable {
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            Log.d(TAG, "Device doesn't have video codec, skipping test");
            return;
        }

        mServer = new CtsTestServer(mContext);
        try {
            // counter must be final if we want to access it inside onTimedMetaData;
            // use AtomicInteger so we can have a final counter object with mutable integer value.
            final AtomicInteger counter = new AtomicInteger();
            String stream_url = mServer.getAssetUrl("prog_index.m3u8");
            mMediaPlayer.setDataSource(stream_url);
            mMediaPlayer.setDisplay(getActivity().getSurfaceHolder());
            mMediaPlayer.setScreenOnWhilePlaying(true);
            mMediaPlayer.setWakeMode(mContext, PowerManager.PARTIAL_WAKE_LOCK);
            mMediaPlayer.setOnTimedMetaDataAvailableListener(new MediaPlayer.OnTimedMetaDataAvailableListener() {
                @Override
                public void onTimedMetaDataAvailable(MediaPlayer mp, TimedMetaData md) {
                    counter.incrementAndGet();
                    int pos = mp.getCurrentPosition();
                    long timeUs = md.getTimestamp();
                    byte[] rawData = md.getMetaData();
                    // Raw data contains an id3 tag holding the decimal string representation of
                    // the associated time stamp rounded to the closest half second.

                    int offset = 0;
                    offset += 3; // "ID3"
                    offset += 2; // version
                    offset += 1; // flags
                    offset += 4; // size
                    offset += 4; // "TXXX"
                    offset += 4; // frame size
                    offset += 2; // frame flags
                    offset += 1; // "\x03" : UTF-8 encoded Unicode
                    offset += 1; // "\x00" : null-terminated empty description

                    int length = rawData.length;
                    length -= offset;
                    length -= 1; // "\x00" : terminating null

                    String data = new String(rawData, offset, length);
                    int dataTimeUs = Integer.parseInt(data);
                    assertTrue("Timed ID3 timestamp does not match content",
                            Math.abs(dataTimeUs - timeUs) < 500000);
                    assertTrue("Timed ID3 arrives after timestamp", pos * 1000 < timeUs);
                }
            });

            final Object completion = new Object();
            mMediaPlayer.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
                int run;
                @Override
                public void onCompletion(MediaPlayer mp) {
                    if (run++ == 0) {
                        mMediaPlayer.seekTo(0);
                        mMediaPlayer.start();
                    } else {
                        mMediaPlayer.stop();
                        synchronized (completion) {
                            completion.notify();
                        }
                    }
                }
            });

            mMediaPlayer.prepare();
            mMediaPlayer.start();
            assertTrue("MediaPlayer not playing", mMediaPlayer.isPlaying());

            int i = -1;
            TrackInfo[] trackInfos = mMediaPlayer.getTrackInfo();
            for (i = 0; i < trackInfos.length; i++) {
                TrackInfo trackInfo = trackInfos[i];
                if (trackInfo.getTrackType() == TrackInfo.MEDIA_TRACK_TYPE_METADATA) {
                    break;
                }
            }
            assertTrue("Stream has no timed ID3 track", i >= 0);
            mMediaPlayer.selectTrack(i);

            synchronized (completion) {
                completion.wait();
            }

            // There are a total of 19 metadata access units in the test stream; every one of them
            // should be received twice: once before the seek and once after.
            assertTrue("Incorrect number of timed ID3s recieved", counter.get() == 38);
        } finally {
            mServer.shutdown();
        }
    }

    private static class WorkerWithPlayer implements Runnable {
        private final Object mLock = new Object();
        private Looper mLooper;
        private MediaPlayer mMediaPlayer;

        /**
         * Creates a worker thread with the given name. The thread
         * then runs a {@link android.os.Looper}.
         * @param name A name for the new thread
         */
        WorkerWithPlayer(String name) {
            Thread t = new Thread(null, this, name);
            t.setPriority(Thread.MIN_PRIORITY);
            t.start();
            synchronized (mLock) {
                while (mLooper == null) {
                    try {
                        mLock.wait();
                    } catch (InterruptedException ex) {
                    }
                }
            }
        }

        public MediaPlayer getPlayer() {
            return mMediaPlayer;
        }

        @Override
        public void run() {
            synchronized (mLock) {
                Looper.prepare();
                mLooper = Looper.myLooper();
                mMediaPlayer = new MediaPlayer();
                mLock.notifyAll();
            }
            Looper.loop();
        }

        public void quit() {
            mLooper.quit();
            mMediaPlayer.release();
        }
    }

    public void testBlockingReadRelease() throws Throwable {

        mServer = new CtsTestServer(mContext);

        WorkerWithPlayer worker = new WorkerWithPlayer("player");
        final MediaPlayer mp = worker.getPlayer();

        try {
            String path = mServer.getDelayedAssetUrl("noiseandchirps.ogg", 15000);
            mp.setDataSource(path);
            mp.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
                @Override
                public void onPrepared(MediaPlayer mp) {
                    fail("prepare should not succeed");
                }
            });
            mp.prepareAsync();
            Thread.sleep(1000);
            long start = SystemClock.elapsedRealtime();
            mp.release();
            long end = SystemClock.elapsedRealtime();
            long releaseDuration = (end - start);
            assertTrue("release took too long: " + releaseDuration, releaseDuration < 1000);
        } catch (IllegalArgumentException e) {
            fail(e.getMessage());
        } catch (SecurityException e) {
            fail(e.getMessage());
        } catch (IllegalStateException e) {
            fail(e.getMessage());
        } catch (IOException e) {
            fail(e.getMessage());
        } catch (InterruptedException e) {
            fail(e.getMessage());
        } finally {
            mServer.shutdown();
        }

        // give the worker a bit of time to start processing the message before shutting it down
        Thread.sleep(5000);
        worker.quit();
    }

    private void localHlsTest(final String name, boolean appendQueryString, boolean redirect)
            throws Throwable {
        mServer = new CtsTestServer(mContext);
        try {
            String stream_url = null;
            if (redirect) {
                stream_url = mServer.getQueryRedirectingAssetUrl(name);
            } else {
                stream_url = mServer.getAssetUrl(name);
            }
            if (appendQueryString) {
                stream_url += "?foo=bar/baz";
            }

            playLiveVideoTest(stream_url, 10);
        } finally {
            mServer.shutdown();
        }
    }
}
