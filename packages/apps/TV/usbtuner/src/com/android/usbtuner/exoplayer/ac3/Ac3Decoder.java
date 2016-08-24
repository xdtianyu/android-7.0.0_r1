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

package com.android.usbtuner.exoplayer.ac3;

import java.nio.ByteBuffer;

/**
 * Decoder for {@link Ac3TrackRenderer}.
 */
public abstract class Ac3Decoder {
    /**
     * Interface definition for AC3 decoder.
     */
    public interface DecodeListener {
        void decodeDone(ByteBuffer resultBuffer, long presentationTimeUs);
    }

    /**
     * Creates {@link AC3Decoder} instance that handles AC3 stream decoder.
     */
    public static Ac3Decoder createAc3Decoder(boolean isSoftware) {
        // TODO: Support framework/software-based AC3 decoder if available.
        return new Ac3PassthroughDecoder();
    }

    public abstract void startDecoder(DecodeListener listener);

    public abstract void decode(ByteBuffer inputBuffer, long presentationTimeUs);
}
