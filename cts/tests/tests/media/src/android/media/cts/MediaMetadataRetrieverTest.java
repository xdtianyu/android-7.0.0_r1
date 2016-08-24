/*
 * Copyright (C) 2013 The Android Open Source Project
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

import android.media.cts.R;

import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.cts.util.MediaUtils;
import android.media.MediaDataSource;
import android.media.MediaMetadataRetriever;
import android.test.AndroidTestCase;

import java.io.IOException;

public class MediaMetadataRetrieverTest extends AndroidTestCase {
    protected Resources mResources;
    protected MediaMetadataRetriever mRetriever;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mResources = getContext().getResources();
        mRetriever = new MediaMetadataRetriever();
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        mRetriever.release();
    }

    protected void setDataSourceFd(int resid) {
        try {
            AssetFileDescriptor afd = mResources.openRawResourceFd(resid);
            mRetriever.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
            afd.close();
        } catch (Exception e) {
            fail("Unable to open file");
        }
    }

    protected TestMediaDataSource setDataSourceCallback(int resid) {
        TestMediaDataSource ds = null;
        try {
            AssetFileDescriptor afd = mResources.openRawResourceFd(resid);
            ds = TestMediaDataSource.fromAssetFd(afd);
            mRetriever.setDataSource(ds);
        } catch (Exception e) {
            fail("Unable to open file");
        }
        return ds;
    }

    public void test3gppMetadata() {
        setDataSourceCallback(R.raw.testvideo);

        assertEquals("Title was other than expected",
                "Title", mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE));

        assertEquals("Artist was other than expected",
                "UTF16LE エンディアン ",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ARTIST));

        assertEquals("Album was other than expected",
                "Test album",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ALBUM));

        assertEquals("Track number was other than expected",
                "10",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_CD_TRACK_NUMBER));

        assertEquals("Year was other than expected",
                "2013", mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_YEAR));

        assertNull("Writer was unexpected present",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_WRITER));
    }

    public void testID3v2Metadata() {
        setDataSourceFd(R.raw.video_480x360_mp4_h264_500kbps_25fps_aac_stereo_128kbps_44100hz_id3v2);

        assertEquals("Title was other than expected",
                "Title", mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE));

        assertEquals("Artist was other than expected",
                "UTF16LE エンディアン ",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ARTIST));

        assertEquals("Album was other than expected",
                "Test album",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ALBUM));

        assertEquals("Track number was other than expected",
                "10",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_CD_TRACK_NUMBER));

        assertEquals("Year was other than expected",
                "2013", mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_YEAR));

        assertNull("Writer was unexpectedly present",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_WRITER));
    }

    public void testSetDataSourceNullPath() {
        try {
            mRetriever.setDataSource((String)null);
            fail("Expected IllegalArgumentException.");
        } catch (IllegalArgumentException ex) {
            // Expected, test passed.
        }
    }

    public void testNullMediaDataSourceIsRejected() {
        try {
            mRetriever.setDataSource((MediaDataSource)null);
            fail("Expected IllegalArgumentException.");
        } catch (IllegalArgumentException ex) {
            // Expected, test passed.
        }
    }

    public void testMediaDataSourceIsClosedOnRelease() throws Exception {
        TestMediaDataSource dataSource = setDataSourceCallback(R.raw.testvideo);
        mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE);
        mRetriever.release();
        assertTrue(dataSource.isClosed());
    }

    public void testRetrieveFailsIfMediaDataSourceThrows() throws Exception {
        TestMediaDataSource dataSource = setDataSourceCallback(R.raw.testvideo);
        dataSource.throwFromReadAt();
        assertTrue(mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE) == null);
    }

    public void testRetrieveFailsIfMediaDataSourceReturnsAnError() throws Exception {
        TestMediaDataSource dataSource = setDataSourceCallback(R.raw.testvideo);
        dataSource.returnFromReadAt(-2);
        assertTrue(mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE) == null);
    }

    private void testThumbnail(int resId) {
        if (!MediaUtils.hasCodecForResourceAndDomain(getContext(), resId, "video/")) {
            MediaUtils.skipTest("no video codecs for resource");
            return;
        }

        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        Resources resources = getContext().getResources();
        AssetFileDescriptor afd = resources.openRawResourceFd(resId);

        retriever.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());

        try {
            afd.close();
        } catch (IOException e) {
            fail("Unable to open file");
        }

        assertNotNull(retriever.getFrameAtTime(-1 /* timeUs (any) */));
    }

    public void testThumbnailH264() {
        testThumbnail(R.raw.bbb_s4_1280x720_mp4_h264_mp31_8mbps_30fps_aac_he_mono_40kbps_44100hz);
    }

    public void testThumbnailH263() {
        testThumbnail(R.raw.video_176x144_3gp_h263_56kbps_12fps_aac_mono_24kbps_11025hz);
    }

    public void testThumbnailMPEG4() {
        testThumbnail(R.raw.video_1280x720_mp4_mpeg4_1000kbps_25fps_aac_stereo_128kbps_44100hz);
    }

    public void testThumbnailVP8() {
        testThumbnail(R.raw.bbb_s1_640x360_webm_vp8_2mbps_30fps_vorbis_5ch_320kbps_48000hz);
    }

    public void testThumbnailVP9() {
        testThumbnail(R.raw.bbb_s1_640x360_webm_vp9_0p21_1600kbps_30fps_vorbis_stereo_128kbps_48000hz);
    }

    public void testThumbnailHEVC() {
        testThumbnail(R.raw.bbb_s1_720x480_mp4_hevc_mp3_1600kbps_30fps_aac_he_6ch_240kbps_48000hz);
    }
}
