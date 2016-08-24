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

package io.appium.droiddriver.actions.accessibility;

import android.annotation.TargetApi;
import android.view.accessibility.AccessibilityNodeInfo;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.exceptions.ActionException;

/**
 * An {@link AccessibilityAction} that clicks on a UiElement.
 */
@TargetApi(18)
public abstract class AccessibilityClickAction extends AccessibilityAction {

  public static final AccessibilityClickAction SINGLE = new SingleClick(1000L);
  public static final AccessibilityClickAction LONG = new LongClick(1000L);
  public static final AccessibilityClickAction DOUBLE = new DoubleClick(1000L);

  protected AccessibilityClickAction(long timeoutMillis) {
    super(timeoutMillis);
  }

  public static class DoubleClick extends AccessibilityClickAction {
    public DoubleClick(long timeoutMillis) {
      super(timeoutMillis);
    }

    @Override
    protected boolean perform(AccessibilityNodeInfo node, UiElement element) {
      return SINGLE.perform(element) && SINGLE.perform(element);
    }
  }

  public static class LongClick extends AccessibilityClickAction {
    public LongClick(long timeoutMillis) {
      super(timeoutMillis);
    }

    @Override
    protected boolean perform(AccessibilityNodeInfo node, UiElement element) {
      if (!element.isLongClickable()) {
        throw new ActionException(element
            + " is not long-clickable; maybe there is a clickable element in the same location?");
      }
      return node.performAction(AccessibilityNodeInfo.ACTION_LONG_CLICK);
    }
  }

  public static class SingleClick extends AccessibilityClickAction {
    public SingleClick(long timeoutMillis) {
      super(timeoutMillis);
    }

    @Override
    protected boolean perform(AccessibilityNodeInfo node, UiElement element) {
      if (!element.isClickable()) {
        throw new ActionException(element
            + " is not clickable; maybe there is a clickable element in the same location?");
      }
      return node.performAction(AccessibilityNodeInfo.ACTION_CLICK);
    }
  }

  @Override
  public String toString() {
    return getClass().getSimpleName();
  }
}
