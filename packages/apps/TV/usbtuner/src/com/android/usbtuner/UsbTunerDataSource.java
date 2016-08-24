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

package com.android.usbtuner;

import android.media.MediaDataSource;
import android.util.Log;

import com.android.usbtuner.ChannelScanFileParser.ScanChannel;
import com.android.usbtuner.data.Channel;
import com.android.usbtuner.data.TunerChannel;
import com.android.usbtuner.tvinput.EventDetector;
import com.android.usbtuner.tvinput.EventDetector.EventListener;
import com.android.usbtuner.tvinput.UsbTunerDebug;

import java.io.IOException;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicLong;

/**
 * A {@link MediaDataSource} implementation which provides the mpeg2ts stream from the tuner device
 * to {@link MediaExtractor}.
 */
public class UsbTunerDataSource extends MediaDataSource implements InputStreamSource {
    private static final String TAG = "UsbTunerDataSource";

    private static final int MIN_READ_UNIT = 1500;
    private static final int READ_BUFFER_SIZE = MIN_READ_UNIT * 10; // ~15KB
    private static final int CIRCULAR_BUFFER_SIZE = MIN_READ_UNIT * 20000;  // ~ 30MB

    private static final int READ_TIMEOUT_MS = 5000; // 5 secs.
    private static final int BUFFER_UNDERRUN_SLEEP_MS = 10;

    private static final int CACHE_KEY_VERSION = 1;

    // UTCK stands for USB Tuner Cache Key.
    private static final String CACHE_KEY_PREFIX = "UTCK";

    private final Object mCircularBufferMonitor = new Object();
    private final byte[] mCircularBuffer = new byte[CIRCULAR_BUFFER_SIZE];
    private long mBytesFetched;
    private final AtomicLong mLastReadPosition = new AtomicLong();
    private boolean mEndOfStreamSent;
    private boolean mStreaming;

    private final TunerHal mTunerHal;
    private Thread mStreamingThread;
    private boolean mDeviceConfigured;
    private EventDetector mEventDetector;

    public UsbTunerDataSource(TunerHal tunerHal, EventListener eventListener) {
        mTunerHal = tunerHal;
        mEventDetector = new EventDetector(mTunerHal, eventListener);
    }

    /**
     * Starts the streaming of a configured program. Throws a runtime exception if no channel and
     * program have successfully been configured yet.
     */
    @Override
    public void startStream() {
        if (!mDeviceConfigured) {
            throw new RuntimeException("Channel and program not configured!");
        }

        synchronized (mCircularBufferMonitor) {
            if (mStreaming) {
                Log.w(TAG, "Streaming should be stopped before start streaming");
                return;
            }
            mStreaming = true;
            mBytesFetched = 0;
            mLastReadPosition.set(0L);
            mEndOfStreamSent = false;
        }

        mStreamingThread = new StreamingThread();
        mStreamingThread.start();
        Log.i(TAG, "Streaming started");
    }

    /**
     * Sets the channel required to start streaming from this device. Afterwards, prepares the tuner
     * device for streaming. Package retrieval can be made at any time after invoking this method
     * and before stopping the stream.
     *
     * @param channel a {@link TunerChannel} instance tune to
     * @return {@code true} if the entire operation was successful; {@code false} otherwise
     */
    @Override
    public boolean tuneToChannel(TunerChannel channel) {
        if (mTunerHal.tune(channel.getFrequency(), channel.getModulation())) {
            if (channel.hasVideo()) {
                mTunerHal.addPidFilter(channel.getVideoPid(),
                        TunerHal.FILTER_TYPE_VIDEO);
            }
            if (channel.hasAudio()) {
                mTunerHal.addPidFilter(channel.getAudioPid(),
                        TunerHal.FILTER_TYPE_AUDIO);
            }
            mTunerHal.addPidFilter(channel.getPcrPid(),
                    TunerHal.FILTER_TYPE_PCR);
            if (mEventDetector != null) {
                mEventDetector.startDetecting(channel.getFrequency(), channel.getModulation());
            }
            mDeviceConfigured = true;
            return true;
        }
        return false;
    }

    /**
     * Blocks the current thread until the streaming thread stops. In rare cases when the tuner
     * device is overloaded this can take a while, but usually it returns pretty quickly.
     */
    @Override
    public void stopStream() {
        synchronized (mCircularBufferMonitor) {
            mStreaming = false;
            mCircularBufferMonitor.notify();
        }

        try {
            if (mStreamingThread != null) {
                mStreamingThread.join();
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        } finally {
            mTunerHal.stopTune();
        }
    }

    @Override
    public long getLimit() {
        synchronized (mCircularBufferMonitor) {
            return mBytesFetched;
        }
    }

    @Override
    public long getPosition() {
        return mLastReadPosition.get();
    }

    private class StreamingThread extends Thread {
        @Override
        public void run() {
            // Buffers for streaming data from the tuner and the internal buffer.
            byte[] dataBuffer = new byte[READ_BUFFER_SIZE];

            while (true) {
                synchronized (mCircularBufferMonitor) {
                    if (!mStreaming) {
                        break;
                    }
                }

                int bytesWritten = mTunerHal.readTsStream(dataBuffer, dataBuffer.length);
                if (bytesWritten <= 0) {
                    try {
                        // When buffer is underrun, we sleep for short time to prevent
                        // unnecessary CPU draining.
                        sleep(BUFFER_UNDERRUN_SLEEP_MS);
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                    continue;
                }

                if (mEventDetector != null) {
                    mEventDetector.feedTSStream(dataBuffer, 0, bytesWritten);
                }
                synchronized (mCircularBufferMonitor) {
                    int posInBuffer = (int) (mBytesFetched % CIRCULAR_BUFFER_SIZE);
                    int bytesToCopyInFirstPass = bytesWritten;
                    if (posInBuffer + bytesToCopyInFirstPass > mCircularBuffer.length) {
                        bytesToCopyInFirstPass = mCircularBuffer.length - posInBuffer;
                    }
                    System.arraycopy(dataBuffer, 0, mCircularBuffer, posInBuffer,
                            bytesToCopyInFirstPass);
                    if (bytesToCopyInFirstPass < bytesWritten) {
                        System.arraycopy(dataBuffer, bytesToCopyInFirstPass, mCircularBuffer, 0,
                                bytesWritten - bytesToCopyInFirstPass);
                    }
                    mBytesFetched += bytesWritten;
                    mCircularBufferMonitor.notify();
                }
            }

            Log.i(TAG, "Streaming stopped");
        }
    }

    @Override
    public int readAt(long pos, byte[] buffer, int offset, int amount) throws IOException {
        synchronized (mCircularBufferMonitor) {
            if (mEndOfStreamSent) {
                // Nothing was received during READ_TIMEOUT_MS before.
                return -1;
            }
            if (mBytesFetched - CIRCULAR_BUFFER_SIZE > pos) {
                // Not available at circular buffer.
                Log.w(TAG, "Not available at circular buffer");
                return -1;
            }
            long initialBytesFetched = mBytesFetched;
            while (mBytesFetched < pos + amount && mStreaming) {
                try {
                    mCircularBufferMonitor.wait(READ_TIMEOUT_MS);
                } catch (InterruptedException e) {
                    // Wait again.
                    Thread.currentThread().interrupt();
                }
                if (initialBytesFetched == mBytesFetched) {
                    Log.w(TAG, "No data update for " + READ_TIMEOUT_MS + "ms. returning -1.");

                    // Returning -1 will make demux report EOS so that the input service can retry
                    // the playback.
                    mEndOfStreamSent = true;
                    return -1;
                }
            }
            if (!mStreaming) {
                Log.w(TAG, "Stream is already stopped.");
                return -1;
            }
            if (mBytesFetched - CIRCULAR_BUFFER_SIZE > pos) {
                Log.e(TAG, "Demux is requesting the data which is already overwritten.");
                return -1;
            }
            int posInBuffer = (int) (pos % CIRCULAR_BUFFER_SIZE);
            int bytesToCopyInFirstPass = amount;
            if (posInBuffer + bytesToCopyInFirstPass > mCircularBuffer.length) {
                bytesToCopyInFirstPass = mCircularBuffer.length - posInBuffer;
            }
            System.arraycopy(mCircularBuffer, posInBuffer, buffer, offset, bytesToCopyInFirstPass);
            if (bytesToCopyInFirstPass < amount) {
                System.arraycopy(mCircularBuffer, 0, buffer, offset + bytesToCopyInFirstPass,
                        amount - bytesToCopyInFirstPass);
            }
            mLastReadPosition.set(pos + amount);
            mCircularBufferMonitor.notify();

            if (UsbTunerDebug.ENABLED) {
                UsbTunerDebug.setBytesInQueue((int) (mBytesFetched - mLastReadPosition.get()));
            }

            return amount;
        }
    }

    @Override
    public long getSize() throws IOException {
        return -1;
    }

    @Override
    public void close() {
        // Called from system MediaExtractor. All the resource should be closed
        // in stopStream() already.
    }

    @Override
    public int getType() {
        return Channel.TYPE_TUNER;
    }

    @Override
    public boolean setScanChannel(ScanChannel channel) {
        return false;
    }

    public static String generateCacheKey(TunerChannel channel, long timestampMs) {
        return String.format(Locale.ENGLISH, "%s-%x-%x-%x-%x", CACHE_KEY_PREFIX, CACHE_KEY_VERSION,
                channel.getFrequency(), channel.getProgramNumber(), timestampMs);
    }

    /**
     * Parses the timestamp from a cache key generated by {@link #generateCacheKey}.
     *
     * @param cacheKey a cache key generated by {@link #generateCacheKey}
     * @return the timestamp parsed from the given cache key. {@code -1} if unable to parse.
     */
    public static long parseTimestampFromCacheKey(String cacheKey) {
        String[] tokens = cacheKey.split("-");
        if (tokens.length < 2 || !tokens[0].equals(CACHE_KEY_PREFIX)) {
            return -1;
        }
        int version = Integer.parseInt(tokens[1], 16);
        if (version == 1) {
            return Long.parseLong(tokens[4], 16);
        } else {
            return -1;
        }
    }
}
