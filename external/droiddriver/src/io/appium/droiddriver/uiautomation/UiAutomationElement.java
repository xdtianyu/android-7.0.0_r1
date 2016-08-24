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
import android.app.UiAutomation;
import android.app.UiAutomation.AccessibilityEventFilter;
import android.graphics.Rect;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

import java.util.ArrayList;
import java.util.Collections;
import java.util.EnumMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeoutException;

import io.appium.droiddriver.actions.InputInjector;
import io.appium.droiddriver.base.BaseUiElement;
import io.appium.droiddriver.finders.Attribute;
import io.appium.droiddriver.uiautomation.UiAutomationContext.UiAutomationCallable;
import io.appium.droiddriver.util.Preconditions;

import static io.appium.droiddriver.util.Strings.charSequenceToString;

/**
 * A UiElement that gets attributes via the Accessibility API.
 */
@TargetApi(18)
public class UiAutomationElement extends BaseUiElement<AccessibilityNodeInfo, UiAutomationElement> {
  private static final AccessibilityEventFilter ANY_EVENT_FILTER = new AccessibilityEventFilter() {
    @Override
    public boolean accept(AccessibilityEvent arg0) {
      return true;
    }
  };

  private final AccessibilityNodeInfo node;
  private final UiAutomationContext context;
  private final Map<Attribute, Object> attributes;
  private final boolean visible;
  private final Rect visibleBounds;
  private final UiAutomationElement parent;
  private final List<UiAutomationElement> children;

  /**
   * A snapshot of all attributes is taken at construction. The attributes of a
   * {@code UiAutomationElement} instance are immutable. If the underlying
   * {@link AccessibilityNodeInfo} is updated, a new {@code UiAutomationElement}
   * instance will be created in
   * {@link io.appium.droiddriver.DroidDriver#refreshUiElementTree}.
   */
  protected UiAutomationElement(UiAutomationContext context, AccessibilityNodeInfo node,
      UiAutomationElement parent) {
    this.node = Preconditions.checkNotNull(node);
    this.context = Preconditions.checkNotNull(context);
    this.parent = parent;

    Map<Attribute, Object> attribs = new EnumMap<Attribute, Object>(Attribute.class);
    put(attribs, Attribute.PACKAGE, charSequenceToString(node.getPackageName()));
    put(attribs, Attribute.CLASS, charSequenceToString(node.getClassName()));
    put(attribs, Attribute.TEXT, charSequenceToString(node.getText()));
    put(attribs, Attribute.CONTENT_DESC, charSequenceToString(node.getContentDescription()));
    put(attribs, Attribute.RESOURCE_ID, charSequenceToString(node.getViewIdResourceName()));
    put(attribs, Attribute.CHECKABLE, node.isCheckable());
    put(attribs, Attribute.CHECKED, node.isChecked());
    put(attribs, Attribute.CLICKABLE, node.isClickable());
    put(attribs, Attribute.ENABLED, node.isEnabled());
    put(attribs, Attribute.FOCUSABLE, node.isFocusable());
    put(attribs, Attribute.FOCUSED, node.isFocused());
    put(attribs, Attribute.LONG_CLICKABLE, node.isLongClickable());
    put(attribs, Attribute.PASSWORD, node.isPassword());
    put(attribs, Attribute.SCROLLABLE, node.isScrollable());
    if (node.getTextSelectionStart() >= 0
        && node.getTextSelectionStart() != node.getTextSelectionEnd()) {
      attribs.put(Attribute.SELECTION_START, node.getTextSelectionStart());
      attribs.put(Attribute.SELECTION_END, node.getTextSelectionEnd());
    }
    put(attribs, Attribute.SELECTED, node.isSelected());
    put(attribs, Attribute.BOUNDS, getBounds(node));
    attributes = Collections.unmodifiableMap(attribs);

    // Order matters as getVisibleBounds depends on visible
    visible = node.isVisibleToUser();
    visibleBounds = getVisibleBounds(node);
    List<UiAutomationElement> mutableChildren = buildChildren(node);
    this.children = mutableChildren == null ? null : Collections.unmodifiableList(mutableChildren);
  }

  private void put(Map<Attribute, Object> attribs, Attribute key, Object value) {
    if (value != null) {
      attribs.put(key, value);
    }
  }

  private List<UiAutomationElement> buildChildren(AccessibilityNodeInfo node) {
    List<UiAutomationElement> children;
    int childCount = node.getChildCount();
    if (childCount == 0) {
      children = null;
    } else {
      children = new ArrayList<UiAutomationElement>(childCount);
      for (int i = 0; i < childCount; i++) {
        AccessibilityNodeInfo child = node.getChild(i);
        if (child != null) {
          children.add(context.getElement(child, this));
        }
      }
    }
    return children;
  }

  private Rect getBounds(AccessibilityNodeInfo node) {
    Rect rect = new Rect();
    node.getBoundsInScreen(rect);
    return rect;
  }

  private Rect getVisibleBounds(AccessibilityNodeInfo node) {
    if (!visible) {
      return new Rect();
    }
    Rect visibleBounds = getBounds();
    UiAutomationElement parent = getParent();
    Rect parentBounds;
    while (parent != null) {
      parentBounds = parent.getBounds();
      visibleBounds.intersect(parentBounds);
      parent = parent.getParent();
    }
    return visibleBounds;
  }

  @Override
  public Rect getVisibleBounds() {
    return visibleBounds;
  }

  @Override
  public boolean isVisible() {
    return visible;
  }

  @Override
  public UiAutomationElement getParent() {
    return parent;
  }

  @Override
  protected List<UiAutomationElement> getChildren() {
    return children;
  }

  @Override
  protected Map<Attribute, Object> getAttributes() {
    return attributes;
  }

  @Override
  public InputInjector getInjector() {
    return context.getDriver().getInjector();
  }

  /**
   * Note: This implementation of {@code doPerformAndWait} clears the
   * {@code AccessibilityEvent} queue.
   */
  @Override
  protected void doPerformAndWait(final FutureTask<Boolean> futureTask, final long timeoutMillis) {
    context.callUiAutomation(new UiAutomationCallable<Void>() {

      @Override
      public Void call(UiAutomation uiAutomation) {
        try {
          uiAutomation.executeAndWaitForEvent(futureTask, ANY_EVENT_FILTER, timeoutMillis);
        } catch (TimeoutException e) {
          // This is for sync'ing with Accessibility API on best-effort because
          // it is not reliable.
          // Exception is ignored here. Tests will fail anyways if this is
          // critical.
          // Actions should usually trigger some AccessibilityEvent's, but some
          // widgets fail to do so, resulting in stale AccessibilityNodeInfo's.
          // As a work-around, force to clear the AccessibilityNodeInfoCache.
          // A legitimate case of no AccessibilityEvent is when scrolling has
          // reached the end, but we cannot tell whether it's legitimate or the
          // widget has bugs, so clearAccessibilityNodeInfoCache anyways.
          context.getDriver().clearAccessibilityNodeInfoCache();
        }
        return null;
      }

    });
  }

  @Override
  public AccessibilityNodeInfo getRawElement() {
    return node;
  }
}
