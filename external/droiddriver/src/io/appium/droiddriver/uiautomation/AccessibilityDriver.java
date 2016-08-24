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

import android.app.Instrumentation;
import android.view.accessibility.AccessibilityNodeInfo;

import io.appium.droiddriver.validators.DefaultAccessibilityValidator;
import io.appium.droiddriver.validators.ExemptRootValidator;
import io.appium.droiddriver.validators.ExemptScrollActionValidator;
import io.appium.droiddriver.validators.ExemptedClassesValidator;
import io.appium.droiddriver.validators.FirstApplicableValidator;
import io.appium.droiddriver.validators.Validator;

/**
 * A UiAutomationDriver that validates accessibility.
 */
public class AccessibilityDriver extends UiAutomationDriver {
  private Validator validator = new FirstApplicableValidator(new ExemptRootValidator(),
      new ExemptScrollActionValidator(), new ExemptedClassesValidator(),
      // TODO: ImageViewValidator
      new DefaultAccessibilityValidator());

  public AccessibilityDriver(Instrumentation instrumentation) {
    super(instrumentation);
  }

  @Override
  protected UiAutomationElement newUiElement(AccessibilityNodeInfo rawElement,
      UiAutomationElement parent) {
    UiAutomationElement newUiElement = super.newUiElement(rawElement, parent);
    newUiElement.setValidator(validator);
    return newUiElement;
  }

  /**
   * Gets the current validator.
   */
  public Validator getValidator() {
    return validator;
  }

  /**
   * Sets the validator to check.
   */
  public void setValidator(Validator validator) {
    this.validator = validator;
  }
}
