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

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.exceptions.ActionException;

/**
 * Base class for {@link Action} that injects key events.
 */
public abstract class KeyAction extends EventAction {
  private final boolean checkFocused;

  protected KeyAction(long timeoutMillis, boolean checkFocused) {
    super(timeoutMillis);
    this.checkFocused = checkFocused;
  }

  protected void maybeCheckFocused(UiElement element) {
    if (checkFocused && element != null && !element.isFocused()) {
      throw new ActionException(element + " is not focused");
    }
  }
}
