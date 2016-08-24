/*
 * Copyright (C) 2015 DroidDriver committers
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

package io.appium.droiddriver.actions.view;

import android.content.Context;
import android.os.Bundle;
import android.os.ResultReceiver;
import android.os.SystemClock;
import android.util.Log;
import android.view.View;
import android.view.inputmethod.InputMethodManager;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.exceptions.ActionException;
import io.appium.droiddriver.exceptions.DroidDriverException;
import io.appium.droiddriver.util.InstrumentationUtils;
import io.appium.droiddriver.util.Logs;

/**
 * Closes soft keyboard. Based on the <a href="https://code.google.com/p/android-test-kit/wiki/Espresso">Espresso</a>
 * code under the same name.
 */
public class CloseKeyboardAction extends ViewAction {
  /** Defaults timeoutMillis to 2000 */
  public static final CloseKeyboardAction DEFAULT_INSTANCE = new CloseKeyboardAction(2000L, 1000L);

  private final long keyboardDismissalDelayMillis;

  /**
   * @param timeoutMillis                the value returned by {@link #getTimeoutMillis}
   * @param keyboardDismissalDelayMillis <a href="https://code.google.com/p/android-test-kit/issues/detail?id=79#c7">a
   *                                     delay for the soft keyboard to finish closing</a>
   */
  public CloseKeyboardAction(long timeoutMillis, long keyboardDismissalDelayMillis) {
    super(timeoutMillis);
    this.keyboardDismissalDelayMillis = keyboardDismissalDelayMillis;
  }

  protected boolean perform(View view, UiElement element) {
    InputMethodManager imm = (InputMethodManager) InstrumentationUtils.getTargetContext()
        .getSystemService(Context.INPUT_METHOD_SERVICE);
    final AtomicInteger resultCodeHolder = new AtomicInteger();
    final CountDownLatch latch = new CountDownLatch(1);

    ResultReceiver resultReceiver = new ResultReceiver(null) {
      @Override
      protected void onReceiveResult(int resultCode, Bundle resultData) {
        resultCodeHolder.set(resultCode);
        latch.countDown();
      }
    };

    if (!imm.hideSoftInputFromWindow(view.getWindowToken(), 0, resultReceiver)) {
      Logs.log(Log.INFO, "InputMethodManager.hideSoftInputFromWindow returned false");
      // Soft keyboard is not shown if hideSoftInputFromWindow returned false
      return true;
    }

    try {
      if (!latch.await(getTimeoutMillis(), TimeUnit.MILLISECONDS)) {
        throw new ActionException("Timed out after " + getTimeoutMillis() + " milliseconds" +
            " waiting for resultCode from InputMethodManager.hideSoftInputFromWindow");
      }
    } catch (InterruptedException e) {
      throw DroidDriverException.propagate(e);
    }

    int resultCode = resultCodeHolder.get();
    if (resultCode != InputMethodManager.RESULT_UNCHANGED_HIDDEN
        && resultCode != InputMethodManager.RESULT_HIDDEN) {
      throw new ActionException("resultCode from InputMethodManager.hideSoftInputFromWindow="
          + resultCode);
    }

    // Wait for the soft keyboard to finish closing
    SystemClock.sleep(keyboardDismissalDelayMillis);
    return true;
  }

  @Override
  public String toString() {
    return getClass().getSimpleName();
  }
}
