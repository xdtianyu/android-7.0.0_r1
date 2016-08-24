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

package com.android.cts.verifier.audio;

import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;
import com.android.cts.verifier.audio.wavelib.*;
import com.android.compatibility.common.util.ReportLog;
import com.android.compatibility.common.util.ResultType;
import com.android.compatibility.common.util.ResultUnit;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;

import android.media.AudioDeviceCallback;
import android.media.AudioDeviceInfo;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.AudioRecord;
import android.media.MediaRecorder;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;

import android.util.Log;

import android.view.View;
import android.view.View.OnClickListener;

import android.widget.Button;
import android.widget.TextView;
import android.widget.SeekBar;
import android.widget.LinearLayout;
import android.widget.ProgressBar;

/**
 * Tests Audio built in Microphone response using external speakers and USB reference microphone.
 */
public class AudioFrequencyMicActivity extends AudioFrequencyActivity implements Runnable,
    AudioRecord.OnRecordPositionUpdateListener {
    private static final String TAG = "AudioFrequencyMicActivity";

    private static final int TEST_STARTED = 900;
    private static final int TEST_ENDED = 901;
    private static final int TEST_MESSAGE = 902;
    private static final int TEST1_MESSAGE = 903;
    private static final int TEST1_ENDED = 904;
    private static final double MIN_ENERGY_BAND_1 = -50.0;          //dB Full Scale
    private static final double MAX_ENERGY_BAND_1_BASE = -60.0;     //dB Full Scale
    private static final double MIN_FRACTION_POINTS_IN_BAND = 0.3;
    private static final double MAX_VAL = Math.pow(2, 15);
    private static final double CLIP_LEVEL = (MAX_VAL-10) / MAX_VAL;

    final OnBtnClickListener mBtnClickListener = new OnBtnClickListener();
    Context mContext;

    Button mHeadsetPortYes;
    Button mHeadsetPortNo;

    Button mSpeakersReady;              //user signal to have connected external speakers
    Button mTest1Button;                //execute test 1
    Button mUsbMicReady;          //user signal to have connected USB Microphone
    Button mTest2Button;                 //user to start test
    String mUsbDevicesInfo;             //usb device info for report
    LinearLayout mLayoutTest1;
    LinearLayout mLayoutTest2a;
    LinearLayout mLayoutTest2b;

    TextView mSpeakerReadyText;
    TextView mTest2Result;
    TextView mUsbStatusText;
    TextView mTest1Result;
    ProgressBar mProgressBar;

    private boolean mIsRecording = false;
    private final Object mRecordingLock = new Object();
    private AudioRecord mRecorder;
    private int mMinRecordBufferSizeInSamples = 0;
    private short[] mAudioShortArray;
    private short[] mAudioShortArray2;

    private final int mBlockSizeSamples = 1024;
    private final int mSamplingRate = 48000;
    private final int mSelectedRecordSource = MediaRecorder.AudioSource.VOICE_RECOGNITION;
    private final int mChannelConfig = AudioFormat.CHANNEL_IN_MONO;
    private final int mAudioFormat = AudioFormat.ENCODING_PCM_16BIT;
    private Thread mRecordThread;

    PipeShort mPipe = new PipeShort(65536);
    SoundPlayerObject mSPlayer;

    private DspBufferComplex mC;
    private DspBufferDouble mData;

    private DspWindow mWindow;
    private DspFftServer mFftServer;
    private VectorAverage mFreqAverageMain = new VectorAverage();

    private VectorAverage mFreqAverageBase = new VectorAverage();
    private VectorAverage mFreqAverageBuiltIn = new VectorAverage();
    private VectorAverage mFreqAverageReference = new VectorAverage();

    private int mCurrentTest = -1;
    int mBands = 4;
    AudioBandSpecs[] bandSpecsArray = new AudioBandSpecs[mBands];
    AudioBandSpecs[] baseBandSpecsArray = new AudioBandSpecs[mBands];

    private class OnBtnClickListener implements OnClickListener {
        @Override
        public void onClick(View v) {
            switch (v.getId()) {
            case R.id.audio_frequency_mic_speakers_ready_btn:
                testSpeakersReady();
                setMaxLevel();
                testMaxLevel();
                break;
            case R.id.audio_frequency_mic_test1_btn:
                startTest1();
                break;
            case R.id.audio_frequency_mic_mic_ready_btn:
                testUSB();
                break;
            case R.id.audio_frequency_mic_test2_btn:
                startTest2();
                break;
            case R.id.audio_general_headset_yes:
                Log.i(TAG, "User confirms Headset Port existence");
                mSpeakersReady.setEnabled(true);
                recordHeasetPortFound(true);
                mHeadsetPortYes.setEnabled(false);
                mHeadsetPortNo.setEnabled(false);
                break;
            case R.id.audio_general_headset_no:
                Log.i(TAG, "User denies Headset Port existence");
                recordHeasetPortFound(false);
                getPassButton().setEnabled(true);
                mHeadsetPortYes.setEnabled(false);
                mHeadsetPortNo.setEnabled(false);
                break;
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.audio_frequency_mic_activity);
        mContext = this;

        mHeadsetPortYes = (Button)findViewById(R.id.audio_general_headset_yes);
        mHeadsetPortYes.setOnClickListener(mBtnClickListener);
        mHeadsetPortNo = (Button)findViewById(R.id.audio_general_headset_no);
        mHeadsetPortNo.setOnClickListener(mBtnClickListener);

        mSpeakerReadyText = (TextView) findViewById(R.id.audio_frequency_mic_speakers_ready_status);

        mSpeakersReady  = (Button)findViewById(R.id.audio_frequency_mic_speakers_ready_btn);
        mSpeakersReady.setOnClickListener(mBtnClickListener);
        mSpeakersReady.setEnabled(false);
        mTest1Button = (Button)findViewById(R.id.audio_frequency_mic_test1_btn);
        mTest1Button.setOnClickListener(mBtnClickListener);
        mTest1Result = (TextView)findViewById(R.id.audio_frequency_mic_results1_text);
        mLayoutTest1 = (LinearLayout) findViewById(R.id.audio_frequency_mic_layout_test1);
        mLayoutTest2a = (LinearLayout) findViewById(R.id.audio_frequency_mic_layout_test2a);
        mLayoutTest2b = (LinearLayout) findViewById(R.id.audio_frequency_mic_layout_test2b);
        mUsbMicReady = (Button)findViewById(R.id.audio_frequency_mic_mic_ready_btn);
        mUsbMicReady.setOnClickListener(mBtnClickListener);

        mUsbStatusText = (TextView)findViewById(R.id.audio_frequency_mic_usb_status);
        mTest2Button = (Button)findViewById(R.id.audio_frequency_mic_test2_btn);
        mTest2Button.setOnClickListener(mBtnClickListener);
        mTest2Result = (TextView)findViewById(R.id.audio_frequency_mic_results_text);
        mProgressBar = (ProgressBar)findViewById(R.id.audio_frequency_mic_progress_bar);
        showWait(false);
        enableLayout(mLayoutTest1, false);
        enableLayout(mLayoutTest2a, false);
        enableLayout(mLayoutTest2b, false);

        mSPlayer = new SoundPlayerObject();
        mSPlayer.setSoundWithResId(getApplicationContext(), R.raw.stereo_mono_white_noise_48);
        mSPlayer.setBalance(0.5f);

        //Init FFT stuff
        mAudioShortArray2 = new short[mBlockSizeSamples*2];
        mData = new DspBufferDouble(mBlockSizeSamples);
        mC = new DspBufferComplex(mBlockSizeSamples);
        mFftServer = new DspFftServer(mBlockSizeSamples);

        int overlap = mBlockSizeSamples / 2;

        mWindow = new DspWindow(DspWindow.WINDOW_HANNING, mBlockSizeSamples, overlap);

        setPassFailButtonClickListeners();
        getPassButton().setEnabled(false);
        setInfoResources(R.string.audio_frequency_mic_test,
                R.string.audio_frequency_mic_info, -1);

        //Init bands for BuiltIn/Reference test
        bandSpecsArray[0] = new AudioBandSpecs(
                50, 500,        /* frequency start,stop */
                4.0, -50,     /* start top,bottom value */
                4.0, -4.0       /* stop top,bottom value */);

        bandSpecsArray[1] = new AudioBandSpecs(
                500,4000,       /* frequency start,stop */
                4.0, -4.0,      /* start top,bottom value */
                4.0, -4.0        /* stop top,bottom value */);

        bandSpecsArray[2] = new AudioBandSpecs(
                4000, 12000,    /* frequency start,stop */
                4.0, -4.0,      /* start top,bottom value */
                5.0, -5.0       /* stop top,bottom value */);

        bandSpecsArray[3] = new AudioBandSpecs(
                12000, 20000,   /* frequency start,stop */
                5.0, -5.0,      /* start top,bottom value */
                5.0, -30.0      /* stop top,bottom value */);

        //Init base bands for silence
        baseBandSpecsArray[0] = new AudioBandSpecs(
                50, 500,        /* frequency start,stop */
                40.0, -50.0,     /* start top,bottom value */
                5.0, -50.0       /* stop top,bottom value */);

        baseBandSpecsArray[1] = new AudioBandSpecs(
                500,4000,       /* frequency start,stop */
                5.0, -50.0,      /* start top,bottom value */
                5.0, -50.0        /* stop top,bottom value */);

        baseBandSpecsArray[2] = new AudioBandSpecs(
                4000, 12000,    /* frequency start,stop */
                5.0, -50.0,      /* start top,bottom value */
                5.0, -50.0       /* stop top,bottom value */);

        baseBandSpecsArray[3] = new AudioBandSpecs(
                12000, 20000,   /* frequency start,stop */
                5.0, -50.0,      /* start top,bottom value */
                5.0, -50.0      /* stop top,bottom value */);

    }

    /**
     * enable test ui elements
     */
    private void enableLayout(LinearLayout layout, boolean enable) {
        for (int i = 0; i < layout.getChildCount(); i++) {
            View view = layout.getChildAt(i);
            view.setEnabled(enable);
        }
    }

    /**
     * show active progress bar
     */
    private void showWait(boolean show) {
        if (show) {
            mProgressBar.setVisibility(View.VISIBLE);
        } else {
            mProgressBar.setVisibility(View.INVISIBLE);
        }
    }

    /**
     *  Start the loopback audio test
     */
    private void startTest1() {
        if (mTestThread != null && !mTestThread.isAlive()) {
            mTestThread = null; //kill it.
        }

        if (mTestThread == null) {
            Log.v(TAG,"Executing test Thread");
            mTestThread = new Thread(mTest1Runnable);
            //getPassButton().setEnabled(false);
            if (!mSPlayer.isAlive())
                mSPlayer.start();
            mTestThread.start();
        } else {
            Log.v(TAG,"test Thread already running.");
        }
    }

    Thread mTestThread;
    Runnable mTest1Runnable = new Runnable() {
        public void run() {
            Message msg = Message.obtain();
            msg.what = TEST_STARTED;
            mMessageHandler.sendMessage(msg);

            setMinLevel();
            sendMessage("Testing Background Environment");
            mCurrentTest = 0;
            mSPlayer.setBalance(0.5f);
            mFreqAverageBase.reset();
            play();

            setMaxLevel();
            sendMessage("Testing Built in Microphone");
            mCurrentTest = 1;
            mFreqAverageBuiltIn.reset();
            mSPlayer.setBalance(0.5f);
            play();

            mCurrentTest = -1;
            sendMessage("Testing Completed");

            Message msg2 = Message.obtain();
            msg2.what = TEST1_ENDED;
            mMessageHandler.sendMessage(msg2);
        }

        private void play() {
            startRecording();
            mSPlayer.play(true);

            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                e.printStackTrace();
                //restore interrupted status
                Thread.currentThread().interrupt();
            }

            mSPlayer.play(false);
            stopRecording();
        }

        private void sendMessage(String str) {
            Message msg = Message.obtain();
            msg.what = TEST1_MESSAGE;
            msg.obj = str;
            mMessageHandler.sendMessage(msg);
        }
    };

    /**
     *  Start the loopback audio test
     */
    private void startTest2() {
        if (mTestThread != null && !mTestThread.isAlive()) {
            mTestThread = null; //kill it.
        }

        if (mTestThread == null) {
            Log.v(TAG,"Executing test2 Thread");
            mTestThread = new Thread(mTest2Runnable);
            //getPassButton().setEnabled(false);
            if (!mSPlayer.isAlive())
                mSPlayer.start();
            mTestThread.start();
        } else {
            Log.v(TAG,"test Thread already running.");
        }
    }

    Runnable mTest2Runnable = new Runnable() {
        public void run() {
            Message msg = Message.obtain();
            msg.what = TEST_STARTED;
            mMessageHandler.sendMessage(msg);

            sendMessage("Testing Reference USB Microphone");
            mCurrentTest = 2;
            mFreqAverageReference.reset();
            mSPlayer.setBalance(0.5f);
            play();

            mCurrentTest = -1;
            sendMessage("Testing Completed");

            Message msg2 = Message.obtain();
            msg2.what = TEST_ENDED;
            mMessageHandler.sendMessage(msg2);
        }

        private void play() {
            startRecording();
            mSPlayer.play(true);

            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                e.printStackTrace();
                //restore interrupted status
                Thread.currentThread().interrupt();
            }

            mSPlayer.play(false);
            stopRecording();
        }

        private void sendMessage(String str) {
            Message msg = Message.obtain();
            msg.what = TEST_MESSAGE;
            msg.obj = str;
            mMessageHandler.sendMessage(msg);
        }
    };

    private Handler mMessageHandler = new Handler() {
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
            case TEST_STARTED:
                showWait(true);
                getPassButton().setEnabled(false);
                break;
            case TEST_ENDED:
                showWait(false);
                computeTest2Results();
                break;
            case TEST1_MESSAGE: {
                    String str = (String)msg.obj;
                    if (str != null) {
                        mTest1Result.setText(str);
                    }
                }
                break;
            case TEST1_ENDED:
                showWait(false);
                computeTest1Results();
                break;
            case TEST_MESSAGE: {
                    String str = (String)msg.obj;
                    if (str != null) {
                        mTest2Result.setText(str);
                    }
                }
                break;
            default:
                Log.e(TAG, String.format("Unknown message: %d", msg.what));
            }
        }
    };

    private class Results {
        private String mLabel;
        public double[] mValuesLog;
        int[] mPointsPerBand = new int[mBands];
        double[] mAverageEnergyPerBand = new double[mBands];
        int[] mInBoundPointsPerBand = new int[mBands];
        public boolean mIsBaseMeasurement = false;
        public Results(String label) {
            mLabel = label;
        }

        //append results
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append(String.format("Channel %s\n", mLabel));
            sb.append("Level in Band 1 : " + (testLevel() ? "OK" :"Not Optimal") +
                    (mIsBaseMeasurement ? " (Base Meas.)" : "") + "\n");
            for (int b = 0; b < mBands; b++) {
                double percent = 0;
                if (mPointsPerBand[b] > 0) {
                    percent = 100.0 * (double) mInBoundPointsPerBand[b] / mPointsPerBand[b];
                }
                sb.append(String.format(
                        " Band %d: Av. Level: %.1f dB InBand: %d/%d (%.1f%%) %s\n",
                        b, mAverageEnergyPerBand[b],
                        mInBoundPointsPerBand[b],
                        mPointsPerBand[b],
                        percent,
                        (testInBand(b) ? "OK" : "Not Optimal")));
            }
            return sb.toString();
        }

        public boolean testLevel() {
            if (mIsBaseMeasurement && mAverageEnergyPerBand[1] <= MAX_ENERGY_BAND_1_BASE) {
                return true;
            } else if (mAverageEnergyPerBand[1] >= MIN_ENERGY_BAND_1) {
                return true;
            }
            return false;
        }

        public boolean testInBand(int b) {
            if (b >= 0 && b < mBands && mPointsPerBand[b] > 0) {
                if ((double) mInBoundPointsPerBand[b] / mPointsPerBand[b] >
                    MIN_FRACTION_POINTS_IN_BAND) {
                        return true;
                }
            }
            return false;
        }

        public boolean testAll() {
            if (!testLevel()) {
                return false;
            }
            for (int b = 0; b < mBands; b++) {
                if (!testInBand(b)) {
                    return false;
                }
            }
            return true;
        }
    }


    /**
     * compute test1 results
     */
    private void computeTest1Results() {

        Results resultsBase = new Results("Base");
        if (computeResultsForVector(mFreqAverageBase, resultsBase, true, baseBandSpecsArray)) {
            appendResultsToScreen(resultsBase.toString(), mTest1Result);
            recordTestResults(resultsBase);
        }

        Results resultsBuiltIn = new Results("BuiltIn");
        if (computeResultsForVector(mFreqAverageBuiltIn, resultsBuiltIn, false, bandSpecsArray)) {
            appendResultsToScreen(resultsBuiltIn.toString(), mTest1Result);
            recordTestResults(resultsBuiltIn);
        }

        //tell user to connect USB Microphone
        appendResultsToScreen("\n\n" +
                getResources().getText(R.string.audio_frequency_mic_connect_mic), mTest1Result);
        enableLayout(mLayoutTest2a, true);
    }

    /**
     * compute test results
     */
    private void computeTest2Results() {
        Results resultsReference = new Results("Reference");
        if (computeResultsForVector(mFreqAverageReference, resultsReference,
                false, bandSpecsArray)) {
            appendResultsToScreen(resultsReference.toString(),mTest2Result);
            recordTestResults(resultsReference);
            getPassButton().setEnabled(true);
        }
    }

    private boolean computeResultsForVector(VectorAverage freqAverage, Results results,
            boolean isBase, AudioBandSpecs[] bandSpecs) {

        results.mIsBaseMeasurement = isBase;
        int points = freqAverage.getSize();
        if (points > 0) {
            //compute vector in db
            double[] values = new double[points];
            freqAverage.getData(values, false);
            results.mValuesLog = new double[points];
            for (int i = 0; i < points; i++) {
                results.mValuesLog[i] = 20 * Math.log10(values[i]);
            }

            int currentBand = 0;
            for (int i = 0; i < points; i++) {
                double freq = (double)mSamplingRate * i / (double)mBlockSizeSamples;
                if (freq > bandSpecs[currentBand].mFreqStop) {
                    currentBand++;
                    if (currentBand >= mBands)
                        break;
                }

                if (freq >= bandSpecs[currentBand].mFreqStart) {
                    results.mAverageEnergyPerBand[currentBand] += results.mValuesLog[i];
                    results.mPointsPerBand[currentBand]++;
                }
            }

            for (int b = 0; b < mBands; b++) {
                if (results.mPointsPerBand[b] > 0) {
                    results.mAverageEnergyPerBand[b] =
                            results.mAverageEnergyPerBand[b] / results.mPointsPerBand[b];
                }
            }

            //set offset relative to band 1 level
            for (int b = 0; b < mBands; b++) {
                bandSpecs[b].setOffset(results.mAverageEnergyPerBand[1]);
            }

            //test points in band.
            currentBand = 0;
            for (int i = 0; i < points; i++) {
                double freq = (double)mSamplingRate * i / (double)mBlockSizeSamples;
                if (freq >  bandSpecs[currentBand].mFreqStop) {
                    currentBand++;
                    if (currentBand >= mBands)
                        break;
                }

                if (freq >= bandSpecs[currentBand].mFreqStart) {
                    double value = results.mValuesLog[i];
                    if (bandSpecs[currentBand].isInBounds(freq, value)) {
                        results.mInBoundPointsPerBand[currentBand]++;
                    }
                }
            }
            return true;
        } else {
            return false;
        }
    }

    //append results
    private void appendResultsToScreen(String str, TextView text) {
        String currentText = text.getText().toString();
        text.setText(currentText + "\n" + str);
    }

    /**
     * Store test results in log
     */
    private void recordTestResults(Results results) {
        String channelLabel = "channel_" + results.mLabel;

        for (int b = 0; b < mBands; b++) {
            String bandLabel = String.format(channelLabel + "_%d", b);
            getReportLog().addValue(
                    bandLabel + "_Level",
                    results.mAverageEnergyPerBand[b],
                    ResultType.HIGHER_BETTER,
                    ResultUnit.NONE);

            getReportLog().addValue(
                    bandLabel + "_pointsinbound",
                    results.mInBoundPointsPerBand[b],
                    ResultType.HIGHER_BETTER,
                    ResultUnit.COUNT);

            getReportLog().addValue(
                    bandLabel + "_pointstotal",
                    results.mPointsPerBand[b],
                    ResultType.NEUTRAL,
                    ResultUnit.COUNT);
        }

        getReportLog().addValues(channelLabel + "_magnitudeSpectrumLog",
                results.mValuesLog,
                ResultType.NEUTRAL,
                ResultUnit.NONE);

        Log.v(TAG, "Results Recorded");
    }

    private void recordHeasetPortFound(boolean found) {
        getReportLog().addValue(
                "User Reported Headset Port",
                found ? 1.0 : 0,
                ResultType.NEUTRAL,
                ResultUnit.NONE);
    }

    private void startRecording() {
        synchronized (mRecordingLock) {
            mIsRecording = true;
        }

        boolean successful = initRecord();
        if (successful) {
            startRecordingForReal();
        } else {
            Log.v(TAG, "Recorder initialization error.");
            synchronized (mRecordingLock) {
                mIsRecording = false;
            }
        }
    }

    private void startRecordingForReal() {
        // start streaming
        if (mRecordThread == null) {
            mRecordThread = new Thread(AudioFrequencyMicActivity.this);
            mRecordThread.setName("FrequencyAnalyzerThread");
        }
        if (!mRecordThread.isAlive()) {
            mRecordThread.start();
        }

        mPipe.flush();

        long startTime = SystemClock.uptimeMillis();
        mRecorder.startRecording();
        if (mRecorder.getRecordingState() != AudioRecord.RECORDSTATE_RECORDING) {
            stopRecording();
            return;
        }
        Log.v(TAG, "Start time: " + (long) (SystemClock.uptimeMillis() - startTime) + " ms");
    }

    private void stopRecording() {
        synchronized (mRecordingLock) {
            stopRecordingForReal();
            mIsRecording = false;
        }
    }

    private void stopRecordingForReal() {

        // stop streaming
        Thread zeThread = mRecordThread;
        mRecordThread = null;
        if (zeThread != null) {
            zeThread.interrupt();
            try {
                zeThread.join();
            } catch(InterruptedException e) {
                //restore interrupted status of recording thread
                zeThread.interrupt();
            }
        }
         // release recording resources
        if (mRecorder != null) {
            mRecorder.stop();
            mRecorder.release();
            mRecorder = null;
        }
    }

    private boolean initRecord() {
        int minRecordBuffSizeInBytes = AudioRecord.getMinBufferSize(mSamplingRate,
                mChannelConfig, mAudioFormat);
        Log.v(TAG,"FrequencyAnalyzer: min buff size = " + minRecordBuffSizeInBytes + " bytes");
        if (minRecordBuffSizeInBytes <= 0) {
            return false;
        }

        mMinRecordBufferSizeInSamples = minRecordBuffSizeInBytes / 2;
        // allocate the byte array to read the audio data

        mAudioShortArray = new short[mMinRecordBufferSizeInSamples];

        Log.v(TAG, "Initiating record:");
        Log.v(TAG, "      using source " + mSelectedRecordSource);
        Log.v(TAG, "      at " + mSamplingRate + "Hz");

        try {
            mRecorder = new AudioRecord(mSelectedRecordSource, mSamplingRate,
                    mChannelConfig, mAudioFormat, 2 * minRecordBuffSizeInBytes);
        } catch (IllegalArgumentException e) {
            return false;
        }
        if (mRecorder.getState() != AudioRecord.STATE_INITIALIZED) {
            mRecorder.release();
            mRecorder = null;
            return false;
        }
        mRecorder.setRecordPositionUpdateListener(this);
        mRecorder.setPositionNotificationPeriod(mBlockSizeSamples / 2);
        return true;
    }

    // ---------------------------------------------------------
    // Implementation of AudioRecord.OnPeriodicNotificationListener
    // --------------------
    public void onPeriodicNotification(AudioRecord recorder) {
        int samplesAvailable = mPipe.availableToRead();
        int samplesNeeded = mBlockSizeSamples;
        if (samplesAvailable >= samplesNeeded) {
            mPipe.read(mAudioShortArray2, 0, samplesNeeded);

            //compute stuff.
            int clipcount = 0;
            double sum = 0;
            double maxabs = 0;
            int i;

            for (i = 0; i < samplesNeeded; i++) {
                double value = mAudioShortArray2[i] / MAX_VAL;
                double valueabs = Math.abs(value);

                if (valueabs > maxabs) {
                    maxabs = valueabs;
                }

                if (valueabs > CLIP_LEVEL) {
                    clipcount++;
                }

                sum += value * value;
                //fft stuff
                mData.mData[i] = value;
            }

            //for the current frame, compute FFT and send to the viewer.

            //apply window and pack as complex for now.
            DspBufferMath.mult(mData, mData, mWindow.mBuffer);
            DspBufferMath.set(mC, mData);
            mFftServer.fft(mC, 1);

            double[] halfMagnitude = new double[mBlockSizeSamples / 2];
            for (i = 0; i < mBlockSizeSamples / 2; i++) {
                halfMagnitude[i] = Math.sqrt(mC.mReal[i] * mC.mReal[i] + mC.mImag[i] * mC.mImag[i]);
            }

            mFreqAverageMain.setData(halfMagnitude, false); //average all of them!

            switch(mCurrentTest) {
                case 0:
                    mFreqAverageBase.setData(halfMagnitude, false);
                    break;
                case 1:
                    mFreqAverageBuiltIn.setData(halfMagnitude, false);
                    break;
                case 2:
                    mFreqAverageReference.setData(halfMagnitude, false);
                    break;
            }
        }
    }

    public void onMarkerReached(AudioRecord track) {
    }

    // ---------------------------------------------------------
    // Implementation of Runnable for the audio recording + playback
    // --------------------
    public void run() {
        Thread thisThread = Thread.currentThread();
        while (!thisThread.isInterrupted()) {
            // read from native recorder
            int nSamplesRead = mRecorder.read(mAudioShortArray, 0, mMinRecordBufferSizeInSamples);
            if (nSamplesRead > 0) {
                mPipe.write(mAudioShortArray, 0, nSamplesRead);
            }
        }
    }

    private void testSpeakersReady() {
        boolean isUsbConnected =
                UsbMicrophoneTester.getIsMicrophoneConnected(getApplicationContext());
        if (isUsbConnected) {
            mSpeakerReadyText.setText(" USB device detected, please remove it");
            enableLayout(mLayoutTest1, false);
            //fail
        } else {
            mSpeakerReadyText.setText(" No USB device detected. OK");
            enableLayout(mLayoutTest1, true);
        }
    }

    private void testUSB() {
        boolean isConnected = UsbMicrophoneTester.getIsMicrophoneConnected(getApplicationContext());
        mUsbDevicesInfo = UsbMicrophoneTester.getUSBDeviceListString(getApplicationContext());

        if (isConnected) {
            mUsbStatusText.setText(
                    getResources().getText(R.string.audio_frequency_mic_mic_ready_text));
            enableLayout(mLayoutTest2b, true);
        } else {
            mUsbStatusText.setText(
                    getResources().getText(R.string.audio_frequency_mic_mic_not_ready_text));
            enableLayout(mLayoutTest2b, false);
        }
    }

}
