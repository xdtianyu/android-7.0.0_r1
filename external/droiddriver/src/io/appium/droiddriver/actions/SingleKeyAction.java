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

package io.appium.droiddriver.actions;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.os.Build;
import android.view.KeyEvent;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.util.Events;
import io.appium.droiddriver.util.Strings;
import io.appium.droiddriver.util.Strings.ToStringHelper;

/**
 * An action to press a single key. While it is convenient for navigating the UI, do not overuse it
 * - the application may interpret key codes in a custom way and, more importantly, application
 * users may not have access to it because the device (physical or virtual keyboard) may not support
 * all key codes.
 */
public class SingleKeyAction extends KeyAction {
  // Common instances for convenience and memory preservation.
  public static final SingleKeyAction MENU = new SingleKeyAction(KeyEvent.KEYCODE_MENU);
  public static final SingleKeyAction SEARCH = new SingleKeyAction(KeyEvent.KEYCODE_SEARCH);
  public static final SingleKeyAction BACK = new SingleKeyAction(KeyEvent.KEYCODE_BACK);
  public static final SingleKeyAction DELETE = new SingleKeyAction(KeyEvent.KEYCODE_DEL);
  /** Requires SDK API 11 or higher */
  @SuppressLint("InlinedApi")
  public static final SingleKeyAction CTRL_MOVE_HOME = new SingleKeyAction(
      KeyEvent.KEYCODE_MOVE_HOME, KeyEvent.META_CTRL_LEFT_ON);
  /** Requires SDK API 11 or higher */
  @SuppressLint("InlinedApi")
  public static final SingleKeyAction CTRL_MOVE_END = new SingleKeyAction(
      KeyEvent.KEYCODE_MOVE_END, KeyEvent.META_CTRL_LEFT_ON);

  private final int keyCode;
  private final int metaState;

  /** Defaults metaState to 0 */
  public SingleKeyAction(int keyCode) {
    this(keyCode, 0);
  }

  /** Defaults timeoutMillis to 100 and checkFocused to false */
  public SingleKeyAction(int keyCode, int metaState) {
    this(keyCode, metaState, 100L, false);
  }

  public SingleKeyAction(int keyCode, int metaState, long timeoutMillis, boolean checkFocused) {
    super(timeoutMillis, checkFocused);
    this.keyCode = keyCode;
    this.metaState = metaState;
  }

  @Override
  public boolean perform(InputInjector injector, UiElement element) {
    maybeCheckFocused(element);

    final long downTime = Events.keyDown(injector, keyCode, metaState);
    Events.keyUp(injector, downTime, keyCode, metaState);

    return true;
  }

  @TargetApi(Build.VERSION_CODES.HONEYCOMB_MR1)
  @Override
  public String toString() {
    String keyCodeString =
        Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB_MR1 ? String.valueOf(keyCode)
            : KeyEvent.keyCodeToString(keyCode);
    ToStringHelper toStringHelper = Strings.toStringHelper(this);
    if (metaState != 0) {
      toStringHelper.add("metaState", metaState);
    }
    return toStringHelper.addValue(keyCodeString).toString();
  }
}
