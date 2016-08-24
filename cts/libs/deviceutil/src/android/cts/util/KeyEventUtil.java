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

package android.cts.util;

import android.app.Instrumentation;
import android.os.Looper;
import android.os.SystemClock;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;

import java.lang.reflect.Field;

/**
 * Utility class to send KeyEvents to TextView bypassing the IME. The code is similar to functions
 * in {@link Instrumentation} and {@link android.test.InstrumentationTestCase} classes. It uses
 * {@link InputMethodManager#dispatchKeyEventFromInputMethod(View, KeyEvent)} to send the events.
 * After sending the events waits for idle.
 */
public class KeyEventUtil {
    private final Instrumentation mInstrumentation;

    public KeyEventUtil(Instrumentation instrumentation) {
        this.mInstrumentation = instrumentation;
    }

    /**
     * Sends the key events corresponding to the text to the app being instrumented.
     *
     * @param targetView View to find the ViewRootImpl and dispatch.
     * @param text The text to be sent. Null value returns immediately.
     */
    public final void sendString(final View targetView, final String text) {
        if (text == null) {
            return;
        }

        KeyEvent[] events = getKeyEvents(text);

        if (events != null) {
            for (int i = 0; i < events.length; i++) {
                // We have to change the time of an event before injecting it because
                // all KeyEvents returned by KeyCharacterMap.getEvents() have the same
                // time stamp and the system rejects too old events. Hence, it is
                // possible for an event to become stale before it is injected if it
                // takes too long to inject the preceding ones.
                sendKey(targetView, KeyEvent.changeTimeRepeat(events[i], SystemClock.uptimeMillis(),
                        0));
            }
        }
    }

    /**
     * Sends a series of key events through instrumentation. For instance:
     * sendKeys(view, KEYCODE_DPAD_LEFT, KEYCODE_DPAD_CENTER).
     *
     * @param targetView View to find the ViewRootImpl and dispatch.
     * @param keys The series of key codes.
     */
    public final void sendKeys(final View targetView, final int...keys) {
        final int count = keys.length;

        for (int i = 0; i < count; i++) {
            try {
                sendKeyDownUp(targetView, keys[i]);
            } catch (SecurityException e) {
                // Ignore security exceptions that are now thrown
                // when trying to send to another app, to retain
                // compatibility with existing tests.
            }
        }
    }

    /**
     * Sends a series of key events through instrumentation. The sequence of keys is a string
     * containing the key names as specified in KeyEvent, without the KEYCODE_ prefix. For
     * instance: sendKeys(view, "DPAD_LEFT A B C DPAD_CENTER"). Each key can be repeated by using
     * the N* prefix. For instance, to send two KEYCODE_DPAD_LEFT, use the following:
     * sendKeys(view, "2*DPAD_LEFT").
     *
     * @param targetView View to find the ViewRootImpl and dispatch.
     * @param keysSequence The sequence of keys.
     */
    public final void sendKeys(final View targetView, final String keysSequence) {
        final String[] keys = keysSequence.split(" ");
        final int count = keys.length;

        for (int i = 0; i < count; i++) {
            String key = keys[i];
            int repeater = key.indexOf('*');

            int keyCount;
            try {
                keyCount = repeater == -1 ? 1 : Integer.parseInt(key.substring(0, repeater));
            } catch (NumberFormatException e) {
                Log.w("ActivityTestCase", "Invalid repeat count: " + key);
                continue;
            }

            if (repeater != -1) {
                key = key.substring(repeater + 1);
            }

            for (int j = 0; j < keyCount; j++) {
                try {
                    final Field keyCodeField = KeyEvent.class.getField("KEYCODE_" + key);
                    final int keyCode = keyCodeField.getInt(null);
                    try {
                        sendKeyDownUp(targetView, keyCode);
                    } catch (SecurityException e) {
                        // Ignore security exceptions that are now thrown
                        // when trying to send to another app, to retain
                        // compatibility with existing tests.
                    }
                } catch (NoSuchFieldException e) {
                    Log.w("ActivityTestCase", "Unknown keycode: KEYCODE_" + key);
                    break;
                } catch (IllegalAccessException e) {
                    Log.w("ActivityTestCase", "Unknown keycode: KEYCODE_" + key);
                    break;
                }
            }
        }
    }

    /**
     * Sends an up and down key events.
     *
     * @param targetView View to find the ViewRootImpl and dispatch.
     * @param key The integer keycode for the event to be send.
     */
    public final void sendKeyDownUp(final View targetView, final int key) {
        sendKey(targetView, new KeyEvent(KeyEvent.ACTION_DOWN, key));
        sendKey(targetView, new KeyEvent(KeyEvent.ACTION_UP, key));
    }

    /**
     * Sends a key event.
     *
     * @param targetView View to find the ViewRootImpl and dispatch.
     * @param event KeyEvent to be send.
     */
    public final void sendKey(final View targetView, final KeyEvent event) {
        validateNotAppThread();

        long downTime = event.getDownTime();
        long eventTime = event.getEventTime();
        int action = event.getAction();
        int code = event.getKeyCode();
        int repeatCount = event.getRepeatCount();
        int metaState = event.getMetaState();
        int deviceId = event.getDeviceId();
        int scancode = event.getScanCode();
        int source = event.getSource();
        int flags = event.getFlags();
        if (source == InputDevice.SOURCE_UNKNOWN) {
            source = InputDevice.SOURCE_KEYBOARD;
        }
        if (eventTime == 0) {
            eventTime = SystemClock.uptimeMillis();
        }
        if (downTime == 0) {
            downTime = eventTime;
        }

        final KeyEvent newEvent = new KeyEvent(downTime, eventTime, action, code, repeatCount,
                metaState, deviceId, scancode, flags, source);

        InputMethodManager imm = targetView.getContext().getSystemService(InputMethodManager.class);
        imm.dispatchKeyEventFromInputMethod(null, newEvent);
        mInstrumentation.waitForIdleSync();
    }

    private KeyEvent[] getKeyEvents(final String text) {
        KeyCharacterMap keyCharacterMap = KeyCharacterMap.load(KeyCharacterMap.VIRTUAL_KEYBOARD);
        return keyCharacterMap.getEvents(text.toCharArray());
    }

    private void validateNotAppThread() {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            throw new RuntimeException(
                    "This method can not be called from the main application thread");
        }
    }
}
