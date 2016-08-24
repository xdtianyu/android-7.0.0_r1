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
 * limitations under the License.
 */

package android.media.cts;

import android.media.cts.R;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.cts.util.MediaUtils;
import android.media.cts.CodecUtils;
import android.media.Image;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;
import android.util.Range;

import com.android.compatibility.common.util.DeviceReportLog;
import com.android.compatibility.common.util.ResultType;
import com.android.compatibility.common.util.ResultUnit;
import com.android.compatibility.common.util.Stat;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.HashMap;

/**
 * Conformance test for decoders on the device.
 *
 * This test will decode test vectors and calculate every decoded frame's md5
 * checksum to see if it matches with the correct md5 value read from a
 * reference file associated with the test vector. Test vector md5 sums are
 * based on the YUV 420 plannar format.
 */
public class DecoderConformanceTest extends MediaPlayerTestBase {
    private static enum Status {
        FAIL,
        PASS,
        SKIP;
    }

    private static final String REPORT_LOG_NAME = "CtsMediaTestCases";
    private static final String TAG = "DecoderConformanceTest";
    private Resources mResources;
    private DeviceReportLog mReportLog;
    private MediaCodec mDecoder;
    private MediaExtractor mExtractor;

    private static final Map<String, String> MIMETYPE_TO_TAG = new HashMap <String, String>() {{
        put(MediaFormat.MIMETYPE_VIDEO_VP9, "vp9");
    }};

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mResources = mContext.getResources();
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
    }


    private List<String> readResourceLines(String fileName) throws Exception {
        int resId = mResources.getIdentifier(fileName, "raw", mContext.getPackageName());
        InputStream is = mContext.getResources().openRawResource(resId);
        BufferedReader in = new BufferedReader(new InputStreamReader(is, "UTF-8"));

        // Read the file line by line.
        List<String> lines = new ArrayList<String>();
        String str;
        while ((str = in.readLine()) != null) {
            int k = str.indexOf(' ');
            String line = k >= 0 ? str.substring(0, k) : str;
            lines.add(line);
        }

        is.close();
        return lines;
    }

    private List<String> readCodecTestVectors(String mime) throws Exception {
        String tag = MIMETYPE_TO_TAG.get(mime);
        String testVectorFileName = tag + "_test_vectors";
        return readResourceLines(testVectorFileName);
    }

    private List<String> readVectorMD5Sums(String mime, String vectorName) throws Exception {
        String tag = MIMETYPE_TO_TAG.get(mime);
        String md5FileName = vectorName + "_" + tag + "_md5";
        return readResourceLines(md5FileName);
    }

    private void releaseMediacodec() {
        try {
            mDecoder.stop();
        } catch (Exception e) {
            Log.e(TAG, "Mediacodec stop exception");
        }

        try {
            mDecoder.release();
            mExtractor.release();
        } catch (Exception e) {
            Log.e(TAG, "Mediacodec release exception");
        }

        mDecoder = null;
        mExtractor = null;
    }

    private Status decodeTestVector(String mime, String decoderName, String vectorName) throws Exception {
        int resId = mResources.getIdentifier(vectorName, "raw", mContext.getPackageName());
        AssetFileDescriptor testFd = mResources.openRawResourceFd(resId);
        mExtractor = new MediaExtractor();
        mExtractor.setDataSource(testFd.getFileDescriptor(), testFd.getStartOffset(),
                                 testFd.getLength());
        mExtractor.selectTrack(0);
        int trackIndex = mExtractor.getSampleTrackIndex();
        MediaFormat format = mExtractor.getTrackFormat(trackIndex);
        mDecoder = MediaCodec.createByCodecName(decoderName);

        MediaCodecInfo codecInfo = mDecoder.getCodecInfo();
        MediaCodecInfo.CodecCapabilities caps = codecInfo.getCapabilitiesForType(mime);
        if (!caps.isFormatSupported(format)) {
            return Status.SKIP;
        }

        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        int decodeFrameCount = 0;
        boolean sawInputEOS = false;
        boolean sawOutputEOS = false;
        final long kTimeOutUs = 5000; // 5ms timeout
        List<String> frameMD5Sums;

        try {
            frameMD5Sums = readVectorMD5Sums(mime, vectorName);
        } catch(Exception e) {
            Log.e(TAG, "Fail to read " + vectorName + "md5sum file");
            return Status.FAIL;
        }

        int expectFrameCount = frameMD5Sums.size();
        mDecoder.configure(format, null /* surface */, null /* crypto */, 0 /* flags */);
        mDecoder.start();

        while (!sawOutputEOS) {
            // handle input
            if (!sawInputEOS) {
                int inputIndex = mDecoder.dequeueInputBuffer(kTimeOutUs);
                if (inputIndex >= 0) {
                    ByteBuffer buffer = mDecoder.getInputBuffer(inputIndex);
                    int sampleSize = mExtractor.readSampleData(buffer, 0);
                    if (sampleSize < 0) {
                        mDecoder.queueInputBuffer(inputIndex, 0, 0, 0,
                                                  MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                        sawInputEOS = true;
                    } else {
                        mDecoder.queueInputBuffer(inputIndex, 0, sampleSize, mExtractor.getSampleTime(), 0);
                        mExtractor.advance();
                    }
                }
            }

            // handle output
            int outputBufIndex = mDecoder.dequeueOutputBuffer(info, kTimeOutUs);
            if (outputBufIndex >= 0) {
                if (info.size > 0) { // Disregard 0-sized buffers at the end.
                    MediaFormat bufferFormat = mDecoder.getOutputFormat(outputBufIndex);
                    int width = bufferFormat.getInteger(MediaFormat.KEY_WIDTH);
                    int height = bufferFormat.getInteger(MediaFormat.KEY_HEIGHT);
                    int colorFmt = bufferFormat.getInteger(MediaFormat.KEY_COLOR_FORMAT);

                    String md5CheckSum = "";
                    try  {
                        Image image = mDecoder.getOutputImage(outputBufIndex);
                        md5CheckSum = CodecUtils.getImageMD5Checksum(image);
                    } catch (Exception e) {
                        Log.e(TAG, "getOutputImage md5CheckSum failed", e);
                        return Status.FAIL;
                    }

                    if (!md5CheckSum.equals(frameMD5Sums.get(decodeFrameCount))) {
                        Log.d(TAG, "Frame " + decodeFrameCount + " md5sum mismatch");
                        return Status.FAIL;
                    }

                    decodeFrameCount++;
                }
                mDecoder.releaseOutputBuffer(outputBufIndex, false /* render */);
                if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                    sawOutputEOS = true;
                }
            } else if (outputBufIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                MediaFormat decOutputFormat = mDecoder.getOutputFormat();
                int width = decOutputFormat.getInteger(MediaFormat.KEY_WIDTH);
                int height = decOutputFormat.getInteger(MediaFormat.KEY_HEIGHT);
                Log.d(TAG, "output format " + decOutputFormat);
            } else if (outputBufIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                Log.i(TAG, "Skip handling MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED");
            } else {
                assertEquals(
                        "decoder.dequeueOutputBuffer() unrecognized return index: " + outputBufIndex,
                        MediaCodec.INFO_TRY_AGAIN_LATER, outputBufIndex);
            }
        }

        if (decodeFrameCount != expectFrameCount) {
            Log.d(TAG, vectorName + " decode frame count not match");
            return Status.FAIL;
        }

        mDecoder.stop();
        mDecoder.release();
        mExtractor.release();
        return Status.PASS;
    }

    void decodeTestVectors(String mime, boolean isGoog) throws Exception {
        MediaFormat format = new MediaFormat();
        format.setString(MediaFormat.KEY_MIME, mime);
        String[] decoderNames = MediaUtils.getDecoderNames(isGoog, format);
        for (String decoderName: decoderNames) {
            List<String> testVectors = readCodecTestVectors(mime);
            for (String vectorName: testVectors) {
                boolean pass = false;
                Log.d(TAG, "Decode vector " + vectorName + " with " + decoderName);
                try {
                    Status stat = decodeTestVector(mime, decoderName, vectorName);
                    if (stat == Status.PASS) {
                        pass = true;
                    } else if (stat == Status.SKIP) {
                        continue;
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Decode " + vectorName + " fail");
                }

                String streamName = "decoder_conformance_test";
                mReportLog = new DeviceReportLog(REPORT_LOG_NAME, streamName);
                mReportLog.addValue("mime", mime, ResultType.NEUTRAL, ResultUnit.NONE);
                mReportLog.addValue("is_goog", isGoog, ResultType.NEUTRAL, ResultUnit.NONE);
                mReportLog.addValue("pass", pass, ResultType.NEUTRAL, ResultUnit.NONE);
                mReportLog.addValue("vector_name", vectorName, ResultType.NEUTRAL, ResultUnit.NONE);
                mReportLog.addValue("decode_name", decoderName, ResultType.NEUTRAL,
                        ResultUnit.NONE);
                mReportLog.submit(getInstrumentation());

                if (!pass) {
                    // Release mediacodec in failure or exception cases.
                    releaseMediacodec();
                }
            }

        }
    }

    /**
     * Test VP9 decoders from vendor.
     */
    public void testVP9Other() throws Exception {
        decodeTestVectors(MediaFormat.MIMETYPE_VIDEO_VP9, false /* isGoog */);
    }

    /**
     * Test Google's VP9 decoder from libvpx.
     */
    public void testVP9Goog() throws Exception {
        decodeTestVectors(MediaFormat.MIMETYPE_VIDEO_VP9, true /* isGoog */);
    }

}
