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

package io.appium.droiddriver.validators;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.actions.Action;

/**
 * Exempts root from validation.
 */
public class ExemptRootValidator implements Validator {
  @Override
  public boolean isApplicable(UiElement element, Action action) {
    return element.getParent() == null; // don't check root
  }

  @Override
  public String validate(UiElement element, Action action) {
    return null;
  }
}
