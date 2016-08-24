/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.car;

import android.content.Context;
import android.content.Intent;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
import android.os.UserHandle;
import android.speech.RecognizerIntent;
import android.util.Log;
import android.view.KeyEvent;

import com.android.car.hal.InputHalService;
import com.android.car.hal.VehicleHal;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintWriter;

public class CarInputService implements CarServiceBase, InputHalService.InputListener {

    public interface KeyEventListener {
        void onKeyEvent(KeyEvent event);
    }

    private static final long VOICE_LONG_PRESS_TIME_MS = 1000;

    private final Context mContext;

    private KeyEventListener mVoiceAssitantKeyListener;
    private KeyEventListener mLongVoiceAssitantKeyListener;
    private long mLastVoiceKeyDownTime = 0;

    private KeyEventListener mInstumentClusterKeyListener;

    private ParcelFileDescriptor mInjectionDeviceFd;

    private int mKeyEventCount = 0;

    public CarInputService(Context context) {
        mContext = context;
    }

    /**
     * Set listener for listening voice assistant key event. Setting to null stops listening.
     * If listener is not set, default behavior will be done for short press.
     * If listener is set, short key press will lead into calling the listener.
     * @param listener
     */
    public void setVoiceAssitantKeyListener(KeyEventListener listener) {
        synchronized (this) {
            mVoiceAssitantKeyListener = listener;
        }
    }

    /**
     * Set listener for listening long voice assistant key event. Setting to null stops listening.
     * If listener is not set, default behavior will be done for long press.
     * If listener is set, short long press will lead into calling the listener.
     * @param listener
     */
    public void setLongVoiceAssitantKeyListener(KeyEventListener listener) {
        synchronized (this) {
            mLongVoiceAssitantKeyListener = listener;
        }
    }

    public void setInstrumentClusterKeyListener(KeyEventListener listener) {
        synchronized (this) {
            mInstumentClusterKeyListener = listener;
        }
    }

    @Override
    public void init() {
        InputHalService hal = VehicleHal.getInstance().getInputHal();
        if (!hal.isKeyInputSupported()) {
            Log.w(CarLog.TAG_INPUT, "Hal does not support key input.");
            return;
        }
        String injectionDevice = mContext.getResources().getString(
                R.string.inputInjectionDeviceNode);
        ParcelFileDescriptor file = null;
        try {
            file = ParcelFileDescriptor.open(new File(injectionDevice),
                    ParcelFileDescriptor.MODE_READ_WRITE);
        } catch (FileNotFoundException e) {
            Log.w(CarLog.TAG_INPUT, "cannot open device for input injection:" + injectionDevice);
            return;
        }
        synchronized (this) {
            mInjectionDeviceFd = file;
        }
        hal.setInputListener(this);
    }

    @Override
    public void release() {
        synchronized (this) {
            mVoiceAssitantKeyListener = null;
            mLongVoiceAssitantKeyListener = null;
            mInstumentClusterKeyListener = null;
            if (mInjectionDeviceFd != null) {
                try {
                    mInjectionDeviceFd.close();
                } catch (IOException e) {
                }
            }
            mInjectionDeviceFd = null;
            mKeyEventCount = 0;
        }
    }

    @Override
    public void onKeyEvent(KeyEvent event, int targetDisplay) {
        synchronized (this) {
            mKeyEventCount++;
        }
        int keyCode = event.getKeyCode();
        switch (keyCode) {
            case KeyEvent.KEYCODE_VOICE_ASSIST:
                handleVoiceAssistKey(event);
                return;
            default:
                break;
        }
        if (targetDisplay == InputHalService.DISPLAY_INSTRUMENT_CLUSTER) {
            handleInstrumentClusterKey(event);
        } else {
            handleMainDisplayKey(event);
        }
    }

    private void handleVoiceAssistKey(KeyEvent event) {
        int action = event.getAction();
        if (action == KeyEvent.ACTION_DOWN) {
            long now = SystemClock.elapsedRealtime();
            synchronized (this) {
                mLastVoiceKeyDownTime = now;
            }
        } else if (action == KeyEvent.ACTION_UP) {
            // if no listener, do not handle long press
            KeyEventListener listener = null;
            KeyEventListener shortPressListener = null;
            KeyEventListener longPressListener = null;
            long downTime;
            synchronized (this) {
                shortPressListener = mVoiceAssitantKeyListener;
                longPressListener = mLongVoiceAssitantKeyListener;
                downTime = mLastVoiceKeyDownTime;
            }
            if (shortPressListener == null && longPressListener == null) {
                launchDefaultVoiceAssitantHandler();
            } else {
                long duration = SystemClock.elapsedRealtime() - downTime;
                listener = (duration > VOICE_LONG_PRESS_TIME_MS
                        ? longPressListener : shortPressListener);
                if (listener != null) {
                    listener.onKeyEvent(event);
                } else {
                    launchDefaultVoiceAssitantHandler();
                }
            }
        }
    }

    private void launchDefaultVoiceAssitantHandler() {
        Log.i(CarLog.TAG_INPUT, "voice key, launch default intent");
        Intent voiceIntent =
                new Intent(RecognizerIntent.ACTION_VOICE_SEARCH_HANDS_FREE);
        mContext.startActivityAsUser(voiceIntent, null, UserHandle.CURRENT_OR_SELF);
    }

    private void handleInstrumentClusterKey(KeyEvent event) {
        KeyEventListener listener = null;
        synchronized (this) {
            listener = mInstumentClusterKeyListener;
        }
        if (listener == null) {
            return;
        }
        listener.onKeyEvent(event);
    }

    private void handleMainDisplayKey(KeyEvent event) {
        int fd;
        synchronized (this) {
            fd = mInjectionDeviceFd.getFd();
        }
        int action = event.getAction();
        boolean isDown = (action == KeyEvent.ACTION_DOWN);
        int keyCode = event.getKeyCode();
        int r = nativeInjectKeyEvent(fd, keyCode, isDown);
        if (r != 0) {
            Log.e(CarLog.TAG_INPUT, "cannot inject key event, failed with:" + r);
        }
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("*Input Service*");
        writer.println("mInjectionDeviceFd:" + mInjectionDeviceFd);
        writer.println("mLastVoiceKeyDownTime:" + mLastVoiceKeyDownTime +
                ",mKeyEventCount:" + mKeyEventCount);
    }

    private native int nativeInjectKeyEvent(int fd, int keyCode, boolean isDown);
}
