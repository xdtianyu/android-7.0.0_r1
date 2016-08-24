/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.speechrecorder;

import java.io.IOException;
import java.io.InputStream;

import android.media.AudioRecord;

import static android.media.AudioFormat.CHANNEL_IN_MONO;
import static android.media.AudioFormat.ENCODING_PCM_16BIT;
import static android.media.MediaRecorder.AudioSource.VOICE_RECOGNITION;

/**
 * Provides an InputStream like API over {@code android.media.AudioRecord}.
 *
 * <b>Not thread safe.</b>
 */
public final class MicrophoneInputStream extends InputStream {

    private final int mSampleRate;
    private final int mBufferSize;
    private final AudioRecord mAudioRecord;

    private boolean mRecording;

    public MicrophoneInputStream(int sampleRate) throws IOException {
        mSampleRate = sampleRate;
        mBufferSize = AudioRecord.getMinBufferSize(sampleRate,
            CHANNEL_IN_MONO, ENCODING_PCM_16BIT);
        mAudioRecord = createAudioRecord();
        mRecording = false;
    }

    private AudioRecord createAudioRecord() throws IOException {
        AudioRecord ar = new AudioRecord(VOICE_RECOGNITION,
            mSampleRate, CHANNEL_IN_MONO, ENCODING_PCM_16BIT, mBufferSize);

        if (ar.getState() != AudioRecord.STATE_INITIALIZED) {
            ar.release();
            throw new IOException("Unable to create AudioRecord");
        }

        return ar;
    }

    private void maybeStartRecording() throws IOException {
        if (mRecording) {
            return;
        }

        mAudioRecord.startRecording();

        int recordingState = mAudioRecord.getRecordingState();
        if (recordingState != AudioRecord.RECORDSTATE_RECORDING) {
            throw new IOException("Unexpected recordingState: " + recordingState);
        }
        mRecording = true;
    }

    @Override
    public int read() throws IOException {
        throw new UnsupportedOperationException("Single byte read.");
    }

    @Override
    public int read(byte[] b, int offset, int length) throws IOException {
        maybeStartRecording();

        final int ret = mAudioRecord.read(b, offset, length);
        if (ret == AudioRecord.ERROR_INVALID_OPERATION ||
                ret == AudioRecord.ERROR_BAD_VALUE) {
            throw new IOException("AudioRecord.read returned: " + ret);
        }

        return ret;
    }

    @Override
    public void close() throws IOException {
        mAudioRecord.stop();
        mAudioRecord.release();
    }
}
