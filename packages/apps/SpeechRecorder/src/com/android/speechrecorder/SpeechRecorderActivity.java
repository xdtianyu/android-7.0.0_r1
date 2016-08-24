/*
 * Copyright (C) 2007 The Android Open Source Project
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

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.TextView;
import java.io.BufferedWriter;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileFilter;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class SpeechRecorderActivity extends Activity {
    private static final String TAG = "SpeechRecorderActivity";

    private static final int DURATION_SEC = 7;

    private Handler mHandler;

    private TextView mCommand;
    private TextView mStatus;
    private Button mRecord;
    private Button mRedo;
    private RadioButton m8KHz;
    private RadioButton m11KHz;
    private RadioButton mCall;
    private RadioButton mDialNanp;
    private RadioButton mDialPairs;

    private InputStream mMicrophone;
    private ByteArrayOutputStream mBaos;

    private File mUtterance;
    private int mSampleRate;
    private Thread mThread;
    private boolean mStoppedListening;

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        mHandler = new Handler();

        setContentView(R.layout.recorder);
        mCommand = (TextView) findViewById(R.id.commandText);
        mStatus = (TextView) findViewById(R.id.statusText);
        mRecord = (Button) findViewById(R.id.recordButton);
        mRedo = (Button) findViewById(R.id.redoButton);
        m8KHz = (RadioButton)findViewById(R.id.codec8KHzRadioButton);
        m11KHz = (RadioButton)findViewById(R.id.codec11KHzRadioButton);
        mCall = (RadioButton)findViewById(R.id.callRadioButton);
        mDialNanp = (RadioButton)findViewById(R.id.dialNanpRadioButton);
        mDialPairs = (RadioButton)findViewById(R.id.dialPairsRadioButton);

        mCommand.setText("Please click 'Record' to begin");
        mRecord.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                if (false) {
                    Log.d(TAG, "mRecord.OnClickListener.onClick");
                }

                setupRecording();
            }
        });

        mRedo.setEnabled(false);
        mRedo.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                if (false) {
                    Log.d(TAG, "mRedo.onClickListener.onClick");
                }

                mUtterance.delete();

                setupRecording();
            }
        });

        m8KHz.setText("PCM/16bit/8KHz");
        m11KHz.setText("PCM/16bit/11KHz");
        m11KHz.setChecked(true);
        mCall.setChecked(true);
    }

    private void setupRecording() {
        Log.d(TAG, "setupRecording");
        // disable buttons
        mRedo.setEnabled(false);
        mRecord.setEnabled(false);
        m8KHz.setFocusable(false);
        m11KHz.setFocusable(false);
        mCall.setFocusable(false);
        mDialNanp.setFocusable(false);
        mDialPairs.setFocusable(false);

        // find the first utterance not covered
        String[] utterances = mCall.isChecked() ? mCallUtterances :
            mDialNanp.isChecked() ? mDialNanpUtterances :
            mDialPairs.isChecked() ? mDialPairsUtterances :
                null;
        mUtterance = null;
        int index = -1;
        for (int i = 0; i < utterances.length; i++) {
            File u = new File(getDir("recordings", MODE_PRIVATE),
                    utterances[i].toLowerCase().replace(' ', '_') + ".wav");
            if (!u.exists()) {
                mUtterance = u;
                index = i;
                break;
            }
        }

        // check if done
        if (mUtterance == null) {
            mCommand.setText("Finished: Thank You!");
            return;
        }
        Log.d(TAG, "going to record " + mUtterance.toString());

        // fix up UI
        mCommand.setText("Say: \"" + utterances[index] + "\"");
        final String status = "item " + (index + 1) + "/" + utterances.length;

        // start the microphone
        mSampleRate = m8KHz.isChecked()? 8000 :
                m11KHz.isChecked() ? 11025 :
                11025;
        mBaos = new ByteArrayOutputStream(mSampleRate * 2 * 20);
        try {
            mMicrophone = new MicrophoneInputStream(mSampleRate);

//            mMicrophone = logInputStream(mUtterance.toString(), mMicrophone, mSampleRate);
        } catch (IOException e) {

        }

        // post a number of delayed events to update the UI and to stop recording
        // after a few seconds.
        for (int i = 0; i <= DURATION_SEC; i++) {
            final int remain = DURATION_SEC - i;
            mHandler.postDelayed(new Runnable() {
                public void run() {
                    if (remain > 0) {
                        mStatus.setText(status + "  Recording... " + remain);
                    }
                    else {
                        mStatus.setText(status);
                        stopRecording();
                    }
                }
            }, i * 1000);
        }

        // now start a thread to store the audio.
        mStoppedListening = false;
        mThread = new Thread() {
            public void run() {
                Log.d(TAG, "run audio capture thread");
                byte buffer[] = new byte[512];
                while (!mStoppedListening) {
                    try {
                        int rtn = 0;
                        rtn = mMicrophone.read(buffer, 0, 512);
                        if (rtn > 0) mBaos.write(buffer, 0, rtn);
                    } catch (IOException e) {
                    }
                }
            }
        };
        mThread.start();

        // to avoid the button click
        try {
            Thread.sleep(100);
        } catch (InterruptedException ie) {
        }

    }

    private void stopRecording() {
        Log.d(TAG, "stopRecording");
        mStoppedListening = true;
        try {
            mThread.join();
        } catch (InterruptedException e) {

        }
        try {
            OutputStream out = new FileOutputStream(mUtterance.toString());
            try {
                byte[] pcm = mBaos.toByteArray();
                Log.d(TAG, "byteArray length " + pcm.length);
                WaveHeader hdr = new WaveHeader(WaveHeader.FORMAT_PCM,
                        (short)1, mSampleRate, (short)16, pcm.length);
                hdr.write(out);
                out.write(pcm);
            } finally {
                out.close();
                mMicrophone.close();
                mBaos.close();
            }
        } catch (IOException e) {


        } finally {
        }

        // stop the recording
        mRecord.setEnabled(true);

        mRedo.setEnabled(true);

        mCommand.setText("Got it!");
    }


    private final static String[] mCallUtterances = new String[] {
        "Call Adam Varro",
        "Call Alex Lloyd",
        "Call Amod Karve",
        "Call Ana Maria Lopez",
        "Call Ben Sigelman",
        "Call Chris Vennard",
        "Call Dana Pogoda",
        "Call Daryl Pregibon",
        "Call Davi Robison",
        "Call David Barrett Kahn",
        "Call David Hyman",
        "Call Douglas Gordin",
        "Call Gregor Rothfuss",
        "Call James Sheridan",
        "Call Jason Charo",
        "Call Jeff Reynar",
        "Call Joel Ward",
        "Call John Milton",
        "Call Lajos Nagy",
        "Call Lori Sobel",
        "Call Martin Jansche",
        "Call Meghan McGarry",
        "Call Meghan Shakar",
        "Call Nilka Thomas",
        "Call Pedro Colijn",
        "Call Pramod Adiddam",
        "Call Rajeev Sivaram",
        "Call Rich Armstrong",
        "Call Robin Watson",
        "Call Sam Morales",
    };

    private final static String[] mDialPairsUtterances = new String[] {
        // all possible pairs
        "Dial 000 000 0000",

        "Dial 101 010 1010",
        "Dial 111 111 1111",

        "Dial 202 020 2020",
        "Dial 212 121 2121",
        "Dial 222 222 2222",

        "Dial 303 030 3030",
        "Dial 313 131 3131",
        "Dial 323 232 3232",
        "Dial 333 333 3333",

        "Dial 404 040 4040",
        "Dial 414 141 4141",
        "Dial 424 242 4242",
        "Dial 434 343 4343",
        "Dial 444 444 4444",

        "Dial 505 050 5050",
        "Dial 515 151 5151",
        "Dial 525 252 5252",
        "Dial 535 353 5353",
        "Dial 545 454 5454",
        "Dial 555 555 5555",

        "Dial 606 060 6060",
        "Dial 616 161 6161",
        "Dial 626 262 6262",
        "Dial 636 363 6363",
        "Dial 646 464 6464",
        "Dial 656 565 6565",
        "Dial 666 666 6666",

        "Dial 707 070 7070",
        "Dial 717 171 7171",
        "Dial 727 272 7272",
        "Dial 737 373 7373",
        "Dial 747 474 7474",
        "Dial 757 575 7575",
        "Dial 767 676 7676",
        "Dial 777 777 7777",

        "Dial 808 080 8080",
        "Dial 818 181 8181",
        "Dial 828 282 8282",
        "Dial 838 383 8383",
        "Dial 848 484 8484",
        "Dial 858 585 8585",
        "Dial 868 686 8686",
        "Dial 878 787 8787",
        "Dial 888 888 8888",

        "Dial 909 090 9090",
        "Dial 919 191 9191",
        "Dial 929 292 9292",
        "Dial 939 393 9393",
        "Dial 949 494 9494",
        "Dial 959 595 9595",
        "Dial 969 696 9696",
        "Dial 979 797 9797",
        "Dial 989 898 9898",
        "Dial 999 999 9999",

    };


    private final static String[] mDialNanpUtterances = new String[] {
        "Dial 211",
        "Dial 411",
        "Dial 511",
        "Dial 811",
        "Dial 911",
        // random numbers
        "Dial 653 5763",
        "Dial 263 9072",
        "Dial 202 9781",
        "Dial 379 8229",
        "Dial 874 9139",
        "Dial 236 0163",
        "Dial 656 7455",
        "Dial 474 5254",
        "Dial 348 8687",
        "Dial 629 8602",

        //"Dial 272 717 8405",
        //"Dial 949 516 0162",
        //"Dial 795 117 7190",
        //"Dial 493 656 3767",
        //"Dial 588 093 9218",
        "Dial 511 658 3690",
        "Dial 440 301 8489",
        "Dial 695 713 6744",
        "Dial 581 475 8712",
        "Dial 981 388 3579",

        "Dial 840 683 3346",
        "Dial 303 467 7988",
        "Dial 649 504 5290",
        "Dial 184 577 4229",
        "Dial 212 286 3982",
        "Dial 646 258 0115",
        "Dial 427 482 6852",
        "Dial 231 809 9260",
        "Dial 681 930 4301",
        "Dial 246 650 8339",
    };
}
