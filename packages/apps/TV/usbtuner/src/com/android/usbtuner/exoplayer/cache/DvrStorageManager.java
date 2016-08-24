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

package com.android.usbtuner.exoplayer.cache;

import android.media.MediaFormat;
import android.util.Pair;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.SortedMap;

/**
 * Manages DVR storage.
 */
public class DvrStorageManager implements CacheManager.StorageManager {

    // TODO: make serializable classes and use protobuf after internal data structure is finalized.
    private static final String KEY_PIXEL_WIDTH_HEIGHT_RATIO =
            "com.google.android.videos.pixelWidthHeightRatio";
    private static final String META_FILE_SUFFIX = ".meta";
    private static final String IDX_FILE_SUFFIX = ".idx";

    // Size of minimum reserved storage buffer which will be used to save meta files
    // and index files after actual recording finished.
    private static final long MIN_BUFFER_BYTES = 256L * 1024 * 1024;
    private static final int NO_VALUE = -1;
    private static final long NO_VALUE_LONG = -1L;

    private final File mCacheDir;

    // {@code true} when this is for recording, {@code false} when this is for replaying.
    private final boolean mIsRecording;

    public DvrStorageManager(File file, boolean isRecording) {
        mCacheDir = file;
        mCacheDir.mkdirs();
        mIsRecording = isRecording;
    }

    @Override
    public void clearStorage() {
        if (mIsRecording) {
            for (File file : mCacheDir.listFiles()) {
                file.delete();
            }
        }
    }

    @Override
    public File getCacheDir() {
        return mCacheDir;
    }

    @Override
    public boolean isPersistent() {
        return true;
    }

    @Override
    public boolean reachedStorageMax(long cacheSize, long pendingDelete) {
        return false;
    }

    @Override
    public boolean hasEnoughBuffer(long pendingDelete) {
        return !mIsRecording || mCacheDir.getUsableSpace() >= MIN_BUFFER_BYTES;
    }

    private void readFormatInt(DataInputStream in, MediaFormat format, String key)
            throws IOException {
        int val = in.readInt();
        if (val != NO_VALUE) {
            format.setInteger(key, val);
        }
    }

    private void readFormatLong(DataInputStream in, MediaFormat format, String key)
            throws IOException {
        long val = in.readLong();
        if (val != NO_VALUE_LONG) {
            format.setLong(key, val);
        }
    }

    private void readFormatFloat(DataInputStream in, MediaFormat format, String key)
            throws IOException {
        float val = in.readFloat();
        if (val != NO_VALUE) {
            format.setFloat(key, val);
        }
    }

    private String readString(DataInputStream in) throws IOException {
        int len = in.readInt();
        if (len <= 0) {
            return null;
        }
        byte [] strBytes = new byte[len];
        in.readFully(strBytes);
        return new String(strBytes, StandardCharsets.UTF_8);
    }

    private void readFormatString(DataInputStream in, MediaFormat format, String key)
            throws IOException {
        String str = readString(in);
        if (str != null) {
            format.setString(key, str);
        }
    }

    private ByteBuffer readByteBuffer(DataInputStream in) throws IOException {
        int len = in.readInt();
        if (len <= 0) {
            return null;
        }
        byte [] bytes = new byte[len];
        in.readFully(bytes);
        ByteBuffer buffer = ByteBuffer.allocate(len);
        buffer.put(bytes);
        buffer.flip();

        return buffer;
    }

    private void readFormatByteBuffer(DataInputStream in, MediaFormat format, String key)
            throws IOException {
        ByteBuffer buffer = readByteBuffer(in);
        if (buffer != null) {
            format.setByteBuffer(key, buffer);
        }
    }

    @Override
    public Pair<String, MediaFormat> readTrackInfoFile(boolean isAudio) throws IOException {
        File file = new File(getCacheDir(), (isAudio ? "audio" : "video") + META_FILE_SUFFIX);
        try (DataInputStream in = new DataInputStream(new FileInputStream(file))) {
            String name = readString(in);
            MediaFormat format = new MediaFormat();
            readFormatString(in, format, MediaFormat.KEY_MIME);
            readFormatInt(in, format, MediaFormat.KEY_MAX_INPUT_SIZE);
            readFormatInt(in, format, MediaFormat.KEY_WIDTH);
            readFormatInt(in, format, MediaFormat.KEY_HEIGHT);
            readFormatInt(in, format, MediaFormat.KEY_CHANNEL_COUNT);
            readFormatInt(in, format, MediaFormat.KEY_SAMPLE_RATE);
            readFormatFloat(in, format, KEY_PIXEL_WIDTH_HEIGHT_RATIO);
            for (int i = 0; i < 3; ++i) {
                readFormatByteBuffer(in, format, "csd-" + i);
            }
            readFormatLong(in, format, MediaFormat.KEY_DURATION);
            return new Pair<>(name, format);
        }
    }

    @Override
    public ArrayList<Long> readIndexFile(String trackId) throws IOException {
        ArrayList<Long> indices = new ArrayList<>();
        File file = new File(getCacheDir(), trackId + IDX_FILE_SUFFIX);
        try (DataInputStream in = new DataInputStream(new FileInputStream(file))) {
            long count = in.readLong();
            for (long i = 0; i < count; ++i) {
                indices.add(in.readLong());
            }
            return indices;
        }
    }

    private void writeFormatInt(DataOutputStream out, MediaFormat format, String key)
            throws IOException {
        if (format.containsKey(key)) {
            out.writeInt(format.getInteger(key));
        } else {
            out.writeInt(NO_VALUE);
        }
    }

    private void writeFormatLong(DataOutputStream out, MediaFormat format, String key)
            throws IOException {
        if (format.containsKey(key)) {
            out.writeLong(format.getLong(key));
        } else {
            out.writeLong(NO_VALUE_LONG);
        }
    }

    private void writeFormatFloat(DataOutputStream out, MediaFormat format, String key)
            throws IOException {
        if (format.containsKey(key)) {
            out.writeFloat(format.getFloat(key));
        } else {
            out.writeFloat(NO_VALUE);
        }
    }

    private void writeString(DataOutputStream out, String str) throws IOException {
        byte [] data = str.getBytes(StandardCharsets.UTF_8);
        out.writeInt(data.length);
        if (data.length > 0) {
            out.write(data);
        }
    }

    private void writeFormatString(DataOutputStream out, MediaFormat format, String key)
            throws IOException {
        if (format.containsKey(key)) {
            writeString(out, format.getString(key));
        } else {
            out.writeInt(0);
        }
    }

    private void writeByteBuffer(DataOutputStream out, ByteBuffer buffer) throws IOException {
        byte [] data = new byte[buffer.limit()];
        buffer.get(data);
        buffer.flip();
        out.writeInt(data.length);
        if (data.length > 0) {
            out.write(data);
        } else {
            out.writeInt(0);
        }
    }

    private void writeFormatByteBuffer(DataOutputStream out, MediaFormat format, String key)
            throws IOException {
        if (format.containsKey(key)) {
            writeByteBuffer(out, format.getByteBuffer(key));
        } else {
            out.writeInt(0);
        }
    }

    @Override
    public void writeTrackInfoFile(String trackId, MediaFormat format, boolean isAudio)
            throws IOException {
        File file = new File(getCacheDir(), (isAudio ? "audio" : "video") + META_FILE_SUFFIX);
        try (DataOutputStream out = new DataOutputStream(new FileOutputStream(file))) {
            writeString(out, trackId);
            writeFormatString(out, format, MediaFormat.KEY_MIME);
            writeFormatInt(out, format, MediaFormat.KEY_MAX_INPUT_SIZE);
            writeFormatInt(out, format, MediaFormat.KEY_WIDTH);
            writeFormatInt(out, format, MediaFormat.KEY_HEIGHT);
            writeFormatInt(out, format, MediaFormat.KEY_CHANNEL_COUNT);
            writeFormatInt(out, format, MediaFormat.KEY_SAMPLE_RATE);
            writeFormatFloat(out, format, KEY_PIXEL_WIDTH_HEIGHT_RATIO);
            for (int i = 0; i < 3; ++i) {
                writeFormatByteBuffer(out, format, "csd-" + i);
            }
            writeFormatLong(out, format, MediaFormat.KEY_DURATION);
        }
    }

    @Override
    public void writeIndexFile(String trackName, SortedMap<Long, SampleCache> index)
            throws IOException {
        File indexFile  = new File(getCacheDir(), trackName + IDX_FILE_SUFFIX);
        try (DataOutputStream out = new DataOutputStream(new FileOutputStream(indexFile))) {
            out.writeLong(index.size());
            for (Long key : index.keySet()) {
                out.writeLong(key);
            }
        }
    }
}
