/*
 * Copyright (C) 2013 DroidDriver committers
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

package io.appium.droiddriver.util;

import android.annotation.TargetApi;
import android.os.Build;
import android.os.SystemClock;
import android.util.Log;
import android.view.InputDevice;
import android.view.InputEvent;
import android.view.KeyEvent;
import android.view.MotionEvent;

import io.appium.droiddriver.actions.InputInjector;
import io.appium.droiddriver.exceptions.ActionException;

/**
 * Helper methods to create InputEvents.
 */
public class Events {
  /**
   * @return a touch down event at the specified coordinates
   */
  @TargetApi(Build.VERSION_CODES.HONEYCOMB_MR1)
  private static MotionEvent newTouchDownEvent(int x, int y) {
    long downTime = SystemClock.uptimeMillis();
    MotionEvent event = MotionEvent.obtain(downTime, downTime, MotionEvent.ACTION_DOWN, x, y, 1);
    // TODO: Fix this if 'source' is required on devices older than HONEYCOMB_MR1.
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR1) {
      event.setSource(InputDevice.SOURCE_TOUCHSCREEN);
    }
    return event;
  }

  /**
   * @return a touch up event at the specified coordinates
   */
  @TargetApi(Build.VERSION_CODES.HONEYCOMB_MR1)
  private static MotionEvent newTouchUpEvent(long downTime, int x, int y) {
    long eventTime = SystemClock.uptimeMillis();
    MotionEvent event = MotionEvent.obtain(downTime, eventTime, MotionEvent.ACTION_UP, x, y, 1);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR1) {
      event.setSource(InputDevice.SOURCE_TOUCHSCREEN);
    }
    return event;
  }

  /**
   * @return a touch move event at the specified coordinates
   */
  @TargetApi(Build.VERSION_CODES.HONEYCOMB_MR1)
  private static MotionEvent newTouchMoveEvent(long downTime, int x, int y) {
    long eventTime = SystemClock.uptimeMillis();
    MotionEvent event = MotionEvent.obtain(downTime, eventTime, MotionEvent.ACTION_MOVE, x, y, 1);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR1) {
      event.setSource(InputDevice.SOURCE_TOUCHSCREEN);
    }
    return event;
  }

  @TargetApi(Build.VERSION_CODES.HONEYCOMB_MR1)
  private static KeyEvent newKeyEvent(long downTime, long eventTime, int action, int keyCode,
      int metaState) {
    KeyEvent event = new KeyEvent(downTime, eventTime, action, keyCode, 0 /* repeat */, metaState);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR1) {
      event.setSource(InputDevice.SOURCE_KEYBOARD);
    }
    return event;
  }

  /**
   * Injects {@code event}. {@code event} is recycled and should not be used
   * after.
   *
   * @throws ActionException if injection failed
   */
  private static void injectEvent(InputInjector injector, InputEvent event) {
    injectEvent(Log.DEBUG, injector, event);
  }

  private static void injectEvent(int priority, InputInjector injector, InputEvent event) {
    Logs.call(priority, injector, "injectInputEvent", event);
    try {
      if (!injector.injectInputEvent(event)) {
        throw new ActionException("Failed to inject " + event);
      }
    } finally {
      if (event instanceof MotionEvent) {
        ((MotionEvent) event).recycle();
      }
    }
  }

  public static long touchDown(InputInjector injector, int x, int y) {
    MotionEvent downEvent = newTouchDownEvent(x, y);
    long downTime = downEvent.getDownTime();
    injectEvent(injector, downEvent);
    return downTime;
  }

  public static void touchUp(InputInjector injector, long downTime, int x, int y) {
    injectEvent(injector, newTouchUpEvent(downTime, x, y));
  }

  public static void touchMove(InputInjector injector, long downTime, int x, int y) {
    injectEvent(Log.VERBOSE, injector, newTouchMoveEvent(downTime, x, y));
  }

  public static long keyDown(InputInjector injector, int keyCode, int metaState) {
    long downTime = SystemClock.uptimeMillis();
    KeyEvent downEvent = newKeyEvent(downTime, downTime, KeyEvent.ACTION_DOWN, keyCode, metaState);
    injectEvent(injector, downEvent);
    return downTime;
  }

  public static void keyUp(InputInjector injector, long downTime, int keyCode, int metaState) {
    injectEvent(injector,
        newKeyEvent(downTime, SystemClock.uptimeMillis(), KeyEvent.ACTION_UP, keyCode, metaState));
  }

  private Events() {}
}
