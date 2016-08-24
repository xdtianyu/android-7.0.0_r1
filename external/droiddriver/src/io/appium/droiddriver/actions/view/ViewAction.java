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

import android.view.View;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.actions.BaseAction;
import io.appium.droiddriver.instrumentation.ViewElement;

/**
 * Implements {@link io.appium.droiddriver.actions.Action} using the associated {@link View}. This
 * can only be used with {@link ViewElement}.
 */
public abstract class ViewAction extends BaseAction {
  protected ViewAction(long timeoutMillis) {
    super(timeoutMillis);
  }

  @Override
  public final boolean perform(UiElement element) {
    return perform(((ViewElement) element).getRawElement(), element);
  }

  /**
   * Performs the action on the associated {@link View}.
   *
   * @param view    the View associated with the UiElement
   * @param element the UiElement to perform the action on
   * @return Whether the action is successful. Some actions throw exceptions in case of failure,
   * when that behavior is more appropriate.
   */
  protected abstract boolean perform(View view, UiElement element);
}
