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

import android.content.Context;
import android.util.Log;

import com.android.tv.common.AutoCloseableUtils;
import com.android.usbtuner.ChannelScanFileParser.ScanChannel;
import com.android.usbtuner.data.Channel;
import com.android.usbtuner.data.TunerChannel;
import com.android.usbtuner.tvinput.EventDetector;
import com.android.usbtuner.tvinput.EventDetector.EventListener;

import java.util.List;
import java.util.concurrent.atomic.AtomicLong;

/**
 * A class that processes mpeg2ts stream coming from a tuner.
 */
public class UsbTunerTsScannerSource implements InputStreamSource {
    // TODO: Refactor with {@link UsbTunerDataSource}.

    private static final String TAG = "UsbTunerTsScannerSource";

    private static final int MIN_READ_UNIT = 1500;
    private static final int READ_BUFFER_SIZE = MIN_READ_UNIT * 10; // ~15KB

    private boolean mStreaming;

    private final TunerHal mTunerHal;
    private Thread mStreamingThread;
    private boolean mDeviceConfigured;
    private final EventDetector mEventDetector;
    private final AtomicLong mBytesFetched = new AtomicLong();

    public UsbTunerTsScannerSource(Context context, EventListener eventListener) {
        mTunerHal = TunerHal.createInstance(context);
        if (mTunerHal == null) {
            throw new RuntimeException("Failed to open a DVB device");
        }
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

        mBytesFetched.set(0L);
        mStreaming = true;
        mStreamingThread = new StreamingThread();
        mStreamingThread.start();
        Log.i(TAG, "Streaming started");
    }

    @Override
    public boolean setScanChannel(ScanChannel channel) {
        if (mTunerHal.tune(channel.frequency, channel.modulation)) {
            mEventDetector.startDetecting(channel.frequency, channel.modulation);
            mDeviceConfigured = true;
            return true;
        }
        return false;
    }

    @Override
    public boolean tuneToChannel(TunerChannel channel) {
        return false;
    }

    public List<TunerChannel> getIncompleteChannels() {
        return mEventDetector.getIncompleteChannels();
    }

    /**
     * Blocks the current thread until the streaming thread stops. In rare cases when the tuner
     * device is overloaded this can take a while, but usually it returns pretty quickly.
     */
    @Override
    public void stopStream() {
        mStreaming = false;

        try {
            if (mStreamingThread != null) {
                mStreamingThread.join();
            }
        } catch (InterruptedException e) {
            Log.e(TAG, "Interrupted while joining the streaming thread.", e);
        }
    }

    @Override
    public long getLimit() {
        return mBytesFetched.get();
    }

    @Override
    public long getPosition() {
        return 0L;
    }

    @Override
    public void close() {
        AutoCloseableUtils.closeQuietly(mTunerHal);
    }

    private class StreamingThread extends Thread {

        @Override
        public void run() {
            // Buffers for streaming data from the tuner and the internal buffer.
            byte[] dataBuffer = new byte[READ_BUFFER_SIZE];

            while (true) {
                if (!mStreaming) {
                    break;
                }
                int bytesWritten;
                bytesWritten = mTunerHal.readTsStream(dataBuffer, dataBuffer.length);
                if (bytesWritten <= 0) {
                    continue;
                }
                mBytesFetched.addAndGet(bytesWritten);

                mEventDetector.feedTSStream(dataBuffer, 0, bytesWritten);
            }

            Log.i(TAG, "Streaming stopped");
        }
    }

    @Override
    public int getType() {
        return Channel.TYPE_TUNER;
    }
}
