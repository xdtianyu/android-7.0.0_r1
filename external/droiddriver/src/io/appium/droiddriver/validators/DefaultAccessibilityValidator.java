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

import android.annotation.TargetApi;
import android.text.TextUtils;
import android.view.accessibility.AccessibilityNodeInfo;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.actions.Action;
import io.appium.droiddriver.uiautomation.UiAutomationElement;

/**
 * Fall-back Validator for accessibility.
 */
@TargetApi(14)
public class DefaultAccessibilityValidator implements Validator {
  @Override
  public boolean isApplicable(UiElement element, Action action) {
    return true;
  }

  @Override
  public String validate(UiElement element, Action action) {
    return isSpeakingNode(element) ? null : "TalkBack cannot speak about it";
  }

  // Logic from TalkBack
  private static boolean isAccessibilityFocusable(UiElement element) {
    if (isActionableForAccessibility(element)) {
      return true;
    }

    if (isTopLevelScrollItem(element) && (isSpeakingNode(element))) {
      return true;
    }
    return false;
  }

  private static boolean isTopLevelScrollItem(UiElement element) {
    UiElement parent = element.getParent();
    return parent != null && parent.isScrollable();
  }

  @SuppressWarnings("deprecation")
  private static boolean isActionableForAccessibility(UiElement element) {
    if (element.isFocusable() || element.isClickable() || element.isLongClickable()) {
      return true;
    }

    if (element instanceof UiAutomationElement) {
      AccessibilityNodeInfo node = ((UiAutomationElement) element).getRawElement();
      return (node.getActions() & AccessibilityNodeInfo.ACTION_FOCUS) == AccessibilityNodeInfo.ACTION_FOCUS;
    }
    return false;
  }

  private static boolean isSpeakingNode(UiElement element) {
    return hasContentDescriptionOrText(element) || element.isCheckable()
        || hasNonActionableSpeakingChildren(element);
  }

  private static boolean hasNonActionableSpeakingChildren(UiElement element) {
    // Recursively check visible and non-focusable descendant nodes.
    for (UiElement child : element.getChildren(UiElement.VISIBLE)) {
      if (!isAccessibilityFocusable(child) && isSpeakingNode(child)) {
        return true;
      }
    }
    return false;
  }

  private static boolean hasContentDescriptionOrText(UiElement element) {
    return !TextUtils.isEmpty(element.getContentDescription())
        || !TextUtils.isEmpty(element.getText());
  }
}
