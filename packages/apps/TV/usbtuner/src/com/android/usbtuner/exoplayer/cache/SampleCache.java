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

import android.os.ConditionVariable;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import com.google.android.exoplayer.SampleHolder;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.channels.FileChannel;
import java.util.concurrent.ConcurrentLinkedQueue;

/**
 * {@link SampleCache} stores samples into file and make them available for read.
 * This class is thread-safe.
 */
public class SampleCache {
    private static final String TAG = "SampleCache";
    private static final boolean DEBUG = false;

    private final long mCreatedTimeMs;
    private final long mStartPositionUs;
    private long mEndPositionUs = 0;
    private SampleCache mNextCache = null;
    private final CacheState mCacheState = new CacheState();
    private final Handler mIoHandler;

    public static class SampleCacheFactory {
        public SampleCache createSampleCache(SamplePool samplePool, File file,
                long startPositionUs, CacheManager.CacheListener cacheListener,
                Looper looper) throws IOException {
            return new SampleCache(samplePool, file, startPositionUs, System.currentTimeMillis(),
                    cacheListener, looper);
        }

        public SampleCache createSampleCacheFromFile(SamplePool samplePool, File cacheDir,
                String filename, long startPositionUs, CacheManager.CacheListener cacheListener,
                Looper looper, SampleCache prev) throws IOException {
            File file = new File(cacheDir, filename);
            SampleCache cache =
                    new SampleCache(samplePool, file, startPositionUs, cacheListener, looper);
            if (prev != null) {
                prev.setNext(cache);
            }
            return cache;
        }
    }

    private static class CacheState {
        private static final int NUM_SAMPLES = 3;

        private volatile boolean mCanReadMore = true;
        private final ConcurrentLinkedQueue<SampleHolder> mSamples = new ConcurrentLinkedQueue<>();
        private volatile long mSize = 0;

        public SampleHolder pollSample() {
            return mSamples.poll();
        }

        public void offerSample(SampleHolder sample) {
            mSamples.offer(sample);
        }

        public void setCanReadMore(boolean canReadMore) {
            mCanReadMore = canReadMore;
        }

        public boolean canReadMore() {
            return mCanReadMore || !mSamples.isEmpty();
        }

        public boolean hasEnoughSamples() {
            return mSamples.size() > NUM_SAMPLES;
        }

        public void setSize(long size) {
            mSize = size;
        }

        public long getSize() {
            return mSize;
        }
    }

    private class IoHandlerCallback implements Handler.Callback {
        public static final int MSG_WRITE = 1;
        public static final int MSG_READ = 2;
        public static final int MSG_CLOSE = 3;
        public static final int MSG_DELETE = 4;
        public static final int MSG_FINISH_WRITE = 5;
        public static final int MSG_RESET_READ = 6;
        public static final int MSG_CLEAR = 7;
        public static final int MSG_OPEN = 8;

        private static final int DELAY_MS = 10;
        private static final int SAMPLE_HEADER_LENGTH = 16;

        private final File mFile;
        private final CacheManager.CacheListener mCacheListener;
        private final SamplePool mSamplePool;
        private final CacheState mCacheState;
        private RandomAccessFile mRaf = null;
        private long mWriteOffset = 0;
        private long mReadOffset = 0;
        private boolean mWriteFinished = false;
        private boolean mDeleteAtEof = false;
        private boolean mDeleted = false;

        public IoHandlerCallback(File file, CacheManager.CacheListener cacheListener,
                SamplePool samplePool, CacheState cacheState, boolean fromFile) throws IOException {
            mFile = file;
            mCacheListener = cacheListener;
            mSamplePool = samplePool;
            mCacheState = cacheState;
            if (fromFile) {
                loadFromFile();
            }
        }

        private void loadFromFile() throws IOException {
            // "r" is enough
            try (RandomAccessFile raf = new RandomAccessFile(mFile, "r")) {
                mWriteFinished = true;
                mWriteOffset = raf.length();
                mCacheState.setSize(mWriteOffset);
                mCacheListener.onWrite(SampleCache.this);
            }
        }

        @Override
        public boolean handleMessage(Message msg) {
            if (mDeleted) {
                if (DEBUG) {
                    Log.d(TAG, "Ignore access to a deleted cache.");
                }
                return true;
            }
            try {
                switch (msg.what) {
                    case MSG_WRITE:
                        handleWrite(msg);
                        return true;
                    case MSG_READ:
                        handleRead(msg);
                        return true;
                    case MSG_CLOSE:
                        handleClose();
                        return true;
                    case MSG_DELETE:
                        handleDelete();
                        return true;
                    case MSG_FINISH_WRITE:
                        handleFinishWrite();
                        return true;
                    case MSG_RESET_READ:
                        handleResetRead();
                        return true;
                    case MSG_CLEAR:
                        handleClear(msg);
                        return true;
                    case MSG_OPEN:
                        handleOpen();
                        return true;
                    default:
                        return false;
                }
            } catch (IOException e) {
                Log.e(TAG, "Error while handling file operation", e);
                return true;
            }
        }

        private void handleWrite(Message msg) throws IOException {
            SampleHolder sample = (SampleHolder) ((Object[])msg.obj)[0];
            ConditionVariable conditionVariable = (ConditionVariable) ((Object[])msg.obj)[1];
            try {
                mRaf.seek(mWriteOffset);
                mRaf.writeInt(sample.size);
                mRaf.writeInt(sample.flags);
                mRaf.writeLong(sample.timeUs);
                sample.data.position(0).limit(sample.size);
                mRaf.getChannel().position(mWriteOffset + SAMPLE_HEADER_LENGTH).write(sample.data);
                mWriteOffset += sample.size + SAMPLE_HEADER_LENGTH;
                mCacheState.setSize(mWriteOffset);
            } finally {
                conditionVariable.open();
            }
        }

        private void handleRead(Message msg) throws IOException {
            msg.getTarget().removeMessages(MSG_READ);
            if (mCacheState.hasEnoughSamples()) {
                // If cache has enough samples, try again few moments later hoping that mCacheState
                // needs a sample by then.
                msg.getTarget().sendEmptyMessageDelayed(MSG_READ, DELAY_MS);
            } else if (mReadOffset >= mWriteOffset) {
                if (mWriteFinished) {
                    if (mRaf != null) {
                        mRaf.close();
                        mRaf = null;
                    }
                    mCacheState.setCanReadMore(false);
                    maybeDelete();
                } else {
                    // Read reached write but write is not finished yet --- wait a few moments to
                    // see if another sample is written.
                    msg.getTarget().sendEmptyMessageDelayed(MSG_READ, DELAY_MS);
                }
            } else {
                if (mRaf == null) {
                    try {
                        mRaf = new RandomAccessFile(mFile, "r");
                    } catch (FileNotFoundException e) {
                        // Cache can be deleted by installd service.
                        Log.e(TAG, "Failed opening a random access file.", e);
                        mDeleted = true;
                        mCacheListener.onDelete(SampleCache.this);
                        return;
                    }
                }
                mRaf.seek(mReadOffset);
                int size = mRaf.readInt();
                SampleHolder sample = mSamplePool.acquireSample(size);
                sample.size = size;
                sample.flags = mRaf.readInt();
                sample.timeUs = mRaf.readLong();
                sample.clearData();
                sample.data.put(mRaf.getChannel().map(FileChannel.MapMode.READ_ONLY,
                        mReadOffset + SAMPLE_HEADER_LENGTH, sample.size));
                mReadOffset += sample.size + SAMPLE_HEADER_LENGTH;
                mCacheState.offerSample(sample);
                msg.getTarget().sendEmptyMessage(MSG_READ);
            }
        }

        private void handleClose() throws IOException {
            if (mWriteFinished) {
                if (mRaf != null) {
                    mRaf.close();
                    mRaf = null;
                }
                mReadOffset = mWriteOffset;
                mCacheState.setCanReadMore(false);
                maybeDelete();
            }
        }

        private void handleDelete() throws IOException {
            mDeleteAtEof = true;
            maybeDelete();
        }

        private void maybeDelete() throws IOException {
            if (!mDeleteAtEof || mCacheState.canReadMore()) {
                return;
            }
            if (mRaf != null) {
                mRaf.close();
                mRaf = null;
            }
            mFile.delete();
            mDeleted = true;
            mCacheListener.onDelete(SampleCache.this);
        }

        private void handleFinishWrite() throws IOException {
            mCacheListener.onWrite(SampleCache.this);
            mWriteFinished = true;
            mRaf.close();
            mRaf = null;
        }

        private void handleResetRead() {
            mReadOffset = 0;
        }

        private void handleClear(Message msg) {
            msg.getTarget().removeMessages(MSG_READ);
            SampleHolder sample;
            while ((sample = mCacheState.pollSample()) != null) {
                mSamplePool.releaseSample(sample);
            }
        }

        private void handleOpen() {
            try {
                mRaf = new RandomAccessFile(mFile, "rw");
            } catch (FileNotFoundException e) {
                Log.e(TAG, "Failed opening a random access file.", e);
            }
        }
    }

    protected SampleCache(SamplePool samplePool, File file, long startPositionUs,
            long createdTimeMs, CacheManager.CacheListener cacheListener, Looper looper)
            throws IOException {
            mEndPositionUs = mStartPositionUs = startPositionUs;
            mCreatedTimeMs = createdTimeMs;
            mIoHandler = new Handler(looper,
                    new IoHandlerCallback(file, cacheListener, samplePool, mCacheState, false));
            mIoHandler.sendEmptyMessage(IoHandlerCallback.MSG_OPEN);
    }

    // Constructor of SampleCache which is backed by the given existing file.
    protected SampleCache(SamplePool samplePool, File file, long startPositionUs,
            CacheManager.CacheListener cacheListener, Looper looper) throws IOException {
        mCreatedTimeMs = mEndPositionUs = mStartPositionUs = startPositionUs;
        IoHandlerCallback handlerCallback =
                new IoHandlerCallback(file, cacheListener, samplePool, mCacheState, true);
        mIoHandler = new Handler(looper, handlerCallback);
    }

    public void resetRead() {
        mCacheState.setCanReadMore(true);
        mIoHandler.sendMessageAtFrontOfQueue(
                mIoHandler.obtainMessage(IoHandlerCallback.MSG_RESET_READ));
    }

    private void setNext(SampleCache next) {
        mNextCache = next;
    }

    public void finishWrite(SampleCache next) {
        setNext(next);
        mIoHandler.sendEmptyMessage(IoHandlerCallback.MSG_FINISH_WRITE);
    }

    public long getStartPositionUs() {
        return mStartPositionUs;
    }

    public SampleCache getNext() {
        return mNextCache;
    }

    public void writeSample(SampleHolder sample, ConditionVariable conditionVariable) {
        if (mNextCache != null) {
            throw new IllegalStateException(
                    "Called writeSample() even though write is already finished");
        }
        mEndPositionUs = sample.timeUs;
        conditionVariable.close();
        mIoHandler.obtainMessage(IoHandlerCallback.MSG_WRITE,
                new Object[] { sample, conditionVariable }).sendToTarget();
    }

    public long getEndPositionUs() {
        return mEndPositionUs;
    }

    public long getCreatedTimeMs() {
        return mCreatedTimeMs;
    }

    public boolean canReadMore() {
        return mCacheState.canReadMore();
    }

    public SampleHolder maybeReadSample() {
        SampleHolder sample = mCacheState.pollSample();
        mIoHandler.sendEmptyMessage(IoHandlerCallback.MSG_READ);
        return sample;
    }

    public void close() {
        mIoHandler.sendEmptyMessage(IoHandlerCallback.MSG_CLOSE);
    }

    public void delete() {
        mIoHandler.sendEmptyMessage(IoHandlerCallback.MSG_DELETE);
    }

    public void clear() {
        mIoHandler.sendMessageAtFrontOfQueue(mIoHandler.obtainMessage(IoHandlerCallback.MSG_CLEAR));
    }

    public long getSize() {
        return mCacheState.getSize();
    }
}
