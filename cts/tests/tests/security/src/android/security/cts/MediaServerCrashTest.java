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

package android.security.cts;

import android.content.res.AssetFileDescriptor;
import android.drm.DrmConvertedStatus;
import android.drm.DrmManagerClient;
import android.media.MediaPlayer;
import android.os.ConditionVariable;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.test.AndroidTestCase;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;

import android.security.cts.R;

public class MediaServerCrashTest extends AndroidTestCase {
    private static final String TAG = "MediaServerCrashTest";

    private static final String MIMETYPE_DRM_MESSAGE = "application/vnd.oma.drm.message";

    private String mFlFilePath;

    private final MediaPlayer mMediaPlayer = new MediaPlayer();
    private final ConditionVariable mOnPrepareCalled = new ConditionVariable();
    private final ConditionVariable mOnCompletionCalled = new ConditionVariable();

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mFlFilePath = new File(Environment.getExternalStorageDirectory(),
                "temp.fl").getAbsolutePath();

        mOnPrepareCalled.close();
        mOnCompletionCalled.close();
        mMediaPlayer.setOnErrorListener(new MediaPlayer.OnErrorListener() {
            @Override
            public boolean onError(MediaPlayer mp, int what, int extra) {
                assertTrue(mp == mMediaPlayer);
                assertTrue("mediaserver process died", what != MediaPlayer.MEDIA_ERROR_SERVER_DIED);
                Log.w(TAG, "onError " + what);
                return false;
            }
        });

        mMediaPlayer.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(MediaPlayer mp) {
                assertTrue(mp == mMediaPlayer);
                mOnPrepareCalled.open();
            }
        });

        mMediaPlayer.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
            @Override
            public void onCompletion(MediaPlayer mp) {
                assertTrue(mp == mMediaPlayer);
                mOnCompletionCalled.open();
            }
        });
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        File flFile = new File(mFlFilePath);
        if (flFile.exists()) {
            flFile.delete();
        }
    }

    public void testInvalidMidiNullPointerAccess() throws Exception {
        testIfMediaServerDied(R.raw.midi_crash);
    }

    private void testIfMediaServerDied(int res) throws Exception {
        AssetFileDescriptor afd = getContext().getResources().openRawResourceFd(res);
        mMediaPlayer.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
        afd.close();
        try {
            mMediaPlayer.prepareAsync();
            if (!mOnPrepareCalled.block(5000)) {
                Log.w(TAG, "testIfMediaServerDied: Timed out waiting for prepare");
                return;
            }
            mMediaPlayer.start();
            if (!mOnCompletionCalled.block(5000)) {
                Log.w(TAG, "testIfMediaServerDied: Timed out waiting for Error/Completion");
            }
        } catch (Exception e) {
            Log.w(TAG, "playback failed", e);
        } finally {
            mMediaPlayer.release();
        }
    }

    public void testDrmManagerClientReset() throws Exception {
        checkIfMediaServerDiedForDrm(R.raw.drm_uaf);
    }

    private void checkIfMediaServerDiedForDrm(int res) throws Exception {
        if (!convertDmToFl(res, mFlFilePath)) {
            Log.w(TAG, "Can not convert dm to fl, skip checkIfMediaServerDiedForDrm");
            mMediaPlayer.release();
            return;
        }
        Log.d(TAG, "intermediate fl file is " + mFlFilePath);

        ParcelFileDescriptor flFd = null;
        try {
            flFd = ParcelFileDescriptor.open(new File(mFlFilePath),
                    ParcelFileDescriptor.MODE_READ_ONLY);
        } catch (FileNotFoundException e) {
            fail("Could not find file: " + mFlFilePath +  e);
        }

        mMediaPlayer.setDataSource(flFd.getFileDescriptor(), 0, flFd.getStatSize());
        flFd.close();
        try {
            mMediaPlayer.prepare();
        } catch (Exception e) {
            Log.d(TAG, "Prepare failed", e);
        }

        try {
            mMediaPlayer.reset();
            if (!mOnCompletionCalled.block(5000)) {
                Log.w(TAG, "checkIfMediaServerDiedForDrm: Timed out waiting for Error/Completion");
            }
        } catch (Exception e) {
            fail("reset failed" + e);
        } finally {
            mMediaPlayer.release();
        }
    }

    private boolean convertDmToFl(int res, String flFilePath) throws Exception {
        AssetFileDescriptor afd = getContext().getResources().openRawResourceFd(res);
        FileInputStream inputStream = afd.createInputStream();
        int inputLength = (int)afd.getLength();
        byte[] fileData = new byte[inputLength];
        int readSize = inputStream.read(fileData, 0, inputLength);
        assertEquals("can not pull in all data", readSize, inputLength);
        inputStream.close();
        afd.close();

        FileOutputStream flStream = new FileOutputStream(new File(flFilePath));

        DrmManagerClient drmClient = null;
        try {
            drmClient = new DrmManagerClient(mContext);
        } catch (IllegalArgumentException e) {
            Log.w(TAG, "DrmManagerClient instance could not be created, context is Illegal.");
            return false;
        } catch (IllegalStateException e) {
            Log.w(TAG, "DrmManagerClient didn't initialize properly.");
            return false;
        }

        if (drmClient == null) {
            Log.w(TAG, "Failed to create DrmManagerClient.");
            return false;
        }

        int convertSessionId = -1;
        try {
            convertSessionId = drmClient.openConvertSession(MIMETYPE_DRM_MESSAGE);
        } catch (IllegalArgumentException e) {
            Log.w(TAG, "Conversion of Mimetype: " + MIMETYPE_DRM_MESSAGE
                    + " is not supported.", e);
            return false;
        } catch (IllegalStateException e) {
            Log.w(TAG, "Could not access Open DrmFramework.", e);
            return false;
        }

        if (convertSessionId < 0) {
            Log.w(TAG, "Failed to open session.");
            return false;
        }

        DrmConvertedStatus convertedStatus = null;
        try {
            convertedStatus = drmClient.convertData(convertSessionId, fileData);
        } catch (IllegalArgumentException e) {
            Log.w(TAG, "Buffer with data to convert is illegal. Convertsession: "
                    + convertSessionId, e);
            return false;
        } catch (IllegalStateException e) {
            Log.w(TAG, "Could not convert data. Convertsession: " + convertSessionId, e);
            return false;
        }

        if (convertedStatus == null ||
                convertedStatus.statusCode != DrmConvertedStatus.STATUS_OK ||
                convertedStatus.convertedData == null) {
            Log.w(TAG, "Error in converting data. Convertsession: " + convertSessionId);
            try {
                drmClient.closeConvertSession(convertSessionId);
            } catch (IllegalStateException e) {
                Log.w(TAG, "Could not close session. Convertsession: " +
                       convertSessionId, e);
            }
            return false;
        }

        flStream.write(convertedStatus.convertedData, 0, convertedStatus.convertedData.length);
        flStream.close();

        try {
            convertedStatus = drmClient.closeConvertSession(convertSessionId);
        } catch (IllegalStateException e) {
            Log.w(TAG, "Could not close convertsession. Convertsession: " +
                    convertSessionId, e);
            return false;
        }

        if (convertedStatus == null ||
                convertedStatus.statusCode != DrmConvertedStatus.STATUS_OK ||
                convertedStatus.convertedData == null) {
            Log.w(TAG, "Error in closing session. Convertsession: " + convertSessionId);
            return false;
        }

        RandomAccessFile flRandomAccessFile = null;
        try {
            flRandomAccessFile = new RandomAccessFile(flFilePath, "rw");
            flRandomAccessFile.seek(convertedStatus.offset);
            flRandomAccessFile.write(convertedStatus.convertedData);
        } catch (FileNotFoundException e) {
            Log.w(TAG, "File: " + flFilePath + " could not be found.", e);
            return false;
        } catch (IOException e) {
            Log.w(TAG, "Could not access File: " + flFilePath + " .", e);
            return false;
        } catch (IllegalArgumentException e) {
            Log.w(TAG, "Could not open file in mode: rw", e);
            return false;
        } catch (SecurityException e) {
            Log.w(TAG, "Access to File: " + flFilePath +
                    " was denied denied by SecurityManager.", e);
            return false;
        } finally {
            if (flRandomAccessFile != null) {
                try {
                    flRandomAccessFile.close();
                } catch (IOException e) {
                    Log.w(TAG, "Failed to close File:" + flFilePath + ".", e);
                    return false;
                }
            }
        }

        return true;
    }
}
