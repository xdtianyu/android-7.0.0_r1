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
import android.app.Instrumentation;
import android.app.UiAutomation;
import android.view.accessibility.AccessibilityNodeInfo;

import io.appium.droiddriver.base.DroidDriverContext;
import io.appium.droiddriver.exceptions.UnrecoverableException;

@TargetApi(18)
public class UiAutomationContext extends
    DroidDriverContext<AccessibilityNodeInfo, UiAutomationElement> {
  private final UiAutomation uiAutomation;

  public UiAutomationContext(Instrumentation instrumentation, UiAutomationDriver driver) {
    super(instrumentation, driver);
    this.uiAutomation = instrumentation.getUiAutomation();
  }

  @Override
  public UiAutomationDriver getDriver() {
    return (UiAutomationDriver) super.getDriver();
  }

  public interface UiAutomationCallable<T> {
    T call(UiAutomation uiAutomation);
  }

  /**
   * Wraps calls to UiAutomation API. Currently supports fail-fast if
   * UiAutomation throws IllegalStateException, which occurs when the connection
   * to UiAutomation service is lost.
   */
  public <T> T callUiAutomation(UiAutomationCallable<T> uiAutomationCallable) {
    try {
      return uiAutomationCallable.call(uiAutomation);
    } catch (IllegalStateException e) {
      throw new UnrecoverableException(e);
    }
  }
}
