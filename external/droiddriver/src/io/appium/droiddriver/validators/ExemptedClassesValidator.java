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

import android.text.TextUtils;
import android.util.Log;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.actions.Action;
import io.appium.droiddriver.util.Logs;

/**
 * Always validates the classes that TalkBack always has speech.
 */
public class ExemptedClassesValidator implements Validator {
  private static final Class<?>[] EXEMPTED_CLASSES = {android.widget.Spinner.class,
      android.widget.EditText.class, android.widget.SeekBar.class,
      android.widget.AbsListView.class, android.widget.TabWidget.class};

  @Override
  public boolean isApplicable(UiElement element, Action action) {
    String className = element.getClassName();
    if (TextUtils.isEmpty(className)) {
      return false;
    }

    Class<?> elementClass = null;
    try {
      elementClass = Class.forName(className);
    } catch (ClassNotFoundException e) {
      Logs.log(Log.WARN, e);
      return false;
    }

    for (Class<?> clazz : EXEMPTED_CLASSES) {
      if (clazz.isAssignableFrom(elementClass)) {
        return true;
      }
    }
    return false;
  }

  @Override
  public String validate(UiElement element, Action action) {
    return null;
  }
}
