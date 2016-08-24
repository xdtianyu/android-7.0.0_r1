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

package io.appium.droiddriver.instrumentation;

import android.app.Instrumentation;
import android.view.InputEvent;
import android.view.KeyEvent;
import android.view.MotionEvent;

import io.appium.droiddriver.actions.InputInjector;
import io.appium.droiddriver.exceptions.ActionException;

public class InstrumentationInputInjector implements InputInjector {
  private final Instrumentation instrumentation;

  public InstrumentationInputInjector(Instrumentation instrumentation) {
    this.instrumentation = instrumentation;
  }

  @Override
  public boolean injectInputEvent(InputEvent event) {
    if (event instanceof MotionEvent) {
      instrumentation.sendPointerSync((MotionEvent) event);
    } else if (event instanceof KeyEvent) {
      instrumentation.sendKeySync((KeyEvent) event);
    } else {
      throw new ActionException("Unknown input event type: " + event);
    }
    return true;
  }
}
