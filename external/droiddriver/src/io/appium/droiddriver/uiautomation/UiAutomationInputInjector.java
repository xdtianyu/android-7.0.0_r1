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

package io.appium.droiddriver.uiautomation;

import android.annotation.TargetApi;
import android.app.UiAutomation;
import android.view.InputEvent;

import io.appium.droiddriver.actions.InputInjector;
import io.appium.droiddriver.uiautomation.UiAutomationContext.UiAutomationCallable;

@TargetApi(18)
public class UiAutomationInputInjector implements InputInjector {
  private final UiAutomationContext context;

  public UiAutomationInputInjector(UiAutomationContext context) {
    this.context = context;
  }

  @Override
  public boolean injectInputEvent(final InputEvent event) {
    return context.callUiAutomation(new UiAutomationCallable<Boolean>() {
      @Override
      public Boolean call(UiAutomation uiAutomation) {
        return uiAutomation.injectInputEvent(event, true /* sync */);
      }
    });
  }
}
