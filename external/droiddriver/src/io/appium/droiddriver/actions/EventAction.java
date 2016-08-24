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

import android.view.InputEvent;

import io.appium.droiddriver.UiElement;

/**
 * Implements {@link Action} by injecting synthesized events.
 */
public abstract class EventAction extends BaseAction {
  protected EventAction(long timeoutMillis) {
    super(timeoutMillis);
  }

  @Override
  public final boolean perform(UiElement element) {
    return perform(element.getInjector(), element);
  }

  /**
   * Performs the action by injecting synthesized events.
   *
   * @param injector the injector to inject {@link InputEvent}s
   * @param element the UiElement to perform the action on
   * @return Whether the action is successful. Some actions throw exceptions in
   *         case of failure, when that behavior is more appropriate. For
   *         example, if event injection returns false.
   */
  protected abstract boolean perform(InputInjector injector, UiElement element);
}
