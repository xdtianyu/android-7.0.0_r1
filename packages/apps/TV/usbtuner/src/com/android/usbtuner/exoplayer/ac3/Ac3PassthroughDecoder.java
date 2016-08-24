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
 * Decodes AC3 audio samples using AC3 Passthrough.
 */
public final class Ac3PassthroughDecoder extends Ac3Decoder {
    private DecodeListener mListener;

    // Should not be created outside the package
    Ac3PassthroughDecoder() {
    }

    @Override
    public void startDecoder(DecodeListener listener) {
        mListener = listener;
    }

    @Override
    public void decode(ByteBuffer inputBuffer, long presentationTimeUs) {
        mListener.decodeDone(inputBuffer, presentationTimeUs);
    }
}
