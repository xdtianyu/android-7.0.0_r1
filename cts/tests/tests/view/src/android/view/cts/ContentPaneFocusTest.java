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

package android.view.cts;

import android.app.ActionBar;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.SystemClock;
import android.test.ActivityInstrumentationTestCase2;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;

public class ContentPaneFocusTest
        extends ActivityInstrumentationTestCase2<ContentPaneCtsActivity> {
    public ContentPaneFocusTest() {
        super("android.view.cts", ContentPaneCtsActivity.class);
    }

    public void testAccessActionBar() throws Throwable {
        final Activity activity = getActivity();

        final View v1 = activity.findViewById(R.id.view1);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                v1.requestFocus();
            }
        });

        getInstrumentation().waitForIdleSync();
        sendControlChar('<');
        getInstrumentation().waitForIdleSync();

        ActionBar action = activity.getActionBar();
        if (action == null || !action.isShowing()) {
            // No action bar, so we only needed to make sure that the shortcut didn't cause
            // the framework to crash.
            return;
        }

        final View content = activity.findViewById(android.R.id.content);
        assertNotNull(content);
        final ViewParent viewParent = content.getParent();
        assertNotNull(viewParent);
        assertTrue(viewParent instanceof ViewGroup);
        ViewGroup parent = (ViewGroup) viewParent;
        View actionBarView = null;
        for (int i = 0; i < parent.getChildCount(); i++) {
            View child = parent.getChildAt(i);
            if ("android:action_bar".equals(child.getTransitionName())) {
                actionBarView = child;
                break;
            }
        }
        assertNotNull(actionBarView);
        final View actionBar = actionBarView;
        // Should jump to the action bar after control-<
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                assertFalse(v1.hasFocus());
                assertTrue(actionBar.hasFocus());
            }
        });
        getInstrumentation().sendKeyDownUpSync(KeyEvent.KEYCODE_DPAD_DOWN);
        getInstrumentation().waitForIdleSync();

        // Should jump to the first view again.
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                assertTrue(v1.hasFocus());
            }
        });
        getInstrumentation().sendKeyDownUpSync(KeyEvent.KEYCODE_DPAD_UP);
        getInstrumentation().waitForIdleSync();

        boolean isTouchScreen = activity.getPackageManager().
                hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN);
        if (isTouchScreen) {
            // Now it shouldn't go up to action bar -- it doesn't allow taking focus once left
            // but only for touch screens.
            runTestOnUiThread(new Runnable() {
                @Override
                public void run() {
                    assertTrue(v1.hasFocus());
                }
            });
        }
    }

    private void sendControlChar(char key) throws Throwable {
        KeyEvent tempEvent = new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_A);
        KeyCharacterMap map = tempEvent.getKeyCharacterMap();
        sendControlKey(KeyEvent.ACTION_DOWN);
        KeyEvent[] events = map.getEvents(new char[] {key});
        final int controlOn = KeyEvent.META_CTRL_ON | KeyEvent.META_CTRL_LEFT_ON;
        for (int i = 0; i < events.length; i++) {
            long time = SystemClock.uptimeMillis();
            KeyEvent event = events[i];
            KeyEvent controlKey = new KeyEvent(time, time, event.getAction(), event.getKeyCode(),
                    event.getRepeatCount(), event.getMetaState() | controlOn);
            getInstrumentation().sendKeySync(controlKey);
            Thread.sleep(2);
        }
        sendControlKey(KeyEvent.ACTION_UP);
    }

    private void sendControlKey(int action) throws Throwable {
        long time = SystemClock.uptimeMillis();
        KeyEvent keyEvent = new KeyEvent(time, time, action, KeyEvent.KEYCODE_CTRL_LEFT, 0,
                KeyEvent.META_CTRL_LEFT_ON | KeyEvent.META_CTRL_ON);
        getInstrumentation().sendKeySync(keyEvent);
        Thread.sleep(2);
    }
}
