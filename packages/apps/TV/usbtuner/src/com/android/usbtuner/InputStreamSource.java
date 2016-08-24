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

import com.android.usbtuner.ChannelScanFileParser.ScanChannel;
import com.android.usbtuner.data.TunerChannel;

/**
 * Interface definition for stream source. Source based on physical tuner or files should implement
 * this interface.
 */
public interface InputStreamSource extends AutoCloseable {
    /**
     * @return a type of the source. Either {@code TYPE_TUNER} or {@code TYPE_FILE}.
     */
    int getType();

    /**
     * Get the source ready to provide stream for channel scanning process.
     *
     * @param channel {@link ScanChannel} to be scanned
     * @return {@code true} if the source is ready, otherwise {@code false}
     */
    boolean setScanChannel(ScanChannel channel);

    /**
     * Tune to a channel to start viewing.
     *
     * @param channel {@link TunerChannel} to view
     * @return {@code true} if tuning was successful, otherwise {@code false}
     */
    boolean tuneToChannel(TunerChannel channel);

    /**
     * Start streaming the data.
     */
    void startStream();

    /**
     * Stop streaming the data.
     */
    void stopStream();

    /**
     * Returns the limit of a input source in bytes. This return value means the number of bytes
     * reading from a tuner device.
     *
     * @return the limit of a input source
     */
    long getLimit();

    /**
     * Returns the position of a input source in bytes. This return value means the number of bytes
     * taken by a reader (for example, {@link MediaExtractor}).
     *
     * @return the position of a input source
     */
    long getPosition();
}
