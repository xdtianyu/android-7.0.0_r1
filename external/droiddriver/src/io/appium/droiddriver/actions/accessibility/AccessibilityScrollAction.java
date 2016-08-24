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
import io.appium.droiddriver.actions.ScrollAction;
import io.appium.droiddriver.scroll.Direction.PhysicalDirection;
import io.appium.droiddriver.util.Strings;

/**
 * An {@link AccessibilityAction} that scrolls an UiElement.
 */
@TargetApi(18)
public class AccessibilityScrollAction extends AccessibilityAction implements ScrollAction {
  private final PhysicalDirection direction;

  public AccessibilityScrollAction(PhysicalDirection direction) {
    this(direction, 1000L);
  }

  public AccessibilityScrollAction(PhysicalDirection direction, long timeoutMillis) {
    super(timeoutMillis);
    this.direction = direction;
  }

  @Override
  protected boolean perform(AccessibilityNodeInfo node, UiElement element) {
    if (!element.isScrollable()) {
      return false;
    }

    switch (direction) {
      case UP:
      case LEFT:
        return node.performAction(AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD);
      case DOWN:
      case RIGHT:
        return node.performAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD);
    }
    return false;
  }

  @Override
  public String toString() {
    return Strings.toStringHelper(this).addValue(direction).toString();
  }
}
