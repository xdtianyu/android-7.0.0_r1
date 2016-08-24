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

import android.graphics.Rect;
import android.os.SystemClock;
import android.view.ViewConfiguration;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.exceptions.ActionException;
import io.appium.droiddriver.scroll.Direction.PhysicalDirection;
import io.appium.droiddriver.util.Events;
import io.appium.droiddriver.util.Strings;
import io.appium.droiddriver.util.Strings.ToStringHelper;

/**
 * An action that swipes the touch screen.
 */
public class SwipeAction extends EventAction implements ScrollAction {
  // Milliseconds between synthesized ACTION_MOVE events.
  // Note: ACTION_MOVE_INTERVAL is the minimum interval between injected events;
  // the actual interval typically is longer.
  private static final int ACTION_MOVE_INTERVAL = 5;
  /**
   * The magic number from UiAutomator. This value is empirical. If it actually
   * results in a fling, you can change it with {@link #setScrollSteps}.
   */
  private static int scrollSteps = 55;
  private static int flingSteps = 3;

  /** Returns the {@link #scrollSteps} used in {@link #toScroll}. */
  public static int getScrollSteps() {
    return scrollSteps;
  }

  /** Sets the {@link #scrollSteps} used in {@link #toScroll}. */
  public static void setScrollSteps(int scrollSteps) {
    SwipeAction.scrollSteps = scrollSteps;
  }

  /** Returns the {@link #flingSteps} used in {@link #toFling}. */
  public static int getFlingSteps() {
    return flingSteps;
  }

  /** Sets the {@link #flingSteps} used in {@link #toFling}. */
  public static void setFlingSteps(int flingSteps) {
    SwipeAction.flingSteps = flingSteps;
  }

  /**
   * Gets {@link SwipeAction} instances for scrolling.
   * <p>
   * Note: This may result in flinging instead of scrolling, depending on the
   * size of the target UiElement and the SDK version of the device. If it does
   * not behave as expected, you can change steps with {@link #setScrollSteps}.
   * </p>
   *
   * @param direction specifies where the view port will move, instead of the
   *        finger.
   * @see ViewConfiguration#getScaledMinimumFlingVelocity
   */
  public static SwipeAction toScroll(PhysicalDirection direction) {
    return new SwipeAction(direction, scrollSteps);
  }

  /**
   * Gets {@link SwipeAction} instances for flinging.
   * <p>
   * Note: This may not actually fling, depending on the size of the target
   * UiElement and the SDK version of the device. If it does not behave as
   * expected, you can change steps with {@link #setFlingSteps}.
   * </p>
   *
   * @param direction specifies where the view port will move, instead of the
   *        finger.
   * @see ViewConfiguration#getScaledMinimumFlingVelocity
   */
  public static SwipeAction toFling(PhysicalDirection direction) {
    return new SwipeAction(direction, flingSteps);
  }

  private final PhysicalDirection direction;
  private final boolean drag;
  private final int steps;
  private final float topMarginRatio;
  private final float leftMarginRatio;
  private final float bottomMarginRatio;
  private final float rightMarginRatio;

  /**
   * Defaults timeoutMillis to 1000 and no drag.
   */
  public SwipeAction(PhysicalDirection direction, int steps) {
    this(direction, steps, false, 1000L);
  }

  /**
   * Defaults all margin ratios to 0.1F.
   */
  public SwipeAction(PhysicalDirection direction, int steps, boolean drag, long timeoutMillis) {
    this(direction, steps, drag, timeoutMillis, 0.1F, 0.1F, 0.1F, 0.1F);
  }

  /**
   * @param direction the scroll direction specifying where the view port will
   *        move, instead of the finger.
   * @param steps minimum 2; (steps-1) is the number of {@code ACTION_MOVE} that
   *        will be injected between {@code ACTION_DOWN} and {@code ACTION_UP}.
   * @param drag whether this is a drag
   * @param timeoutMillis the value returned by {@link #getTimeoutMillis}
   * @param topMarginRatio margin ratio from top
   * @param leftMarginRatio margin ratio from left
   * @param bottomMarginRatio margin ratio from bottom
   * @param rightMarginRatio margin ratio from right
   */
  public SwipeAction(PhysicalDirection direction, int steps, boolean drag, long timeoutMillis,
      float topMarginRatio, float leftMarginRatio, float bottomMarginRatio, float rightMarginRatio) {
    super(timeoutMillis);
    this.direction = direction;
    this.steps = Math.max(2, steps);
    this.drag = drag;
    this.topMarginRatio = topMarginRatio;
    this.bottomMarginRatio = bottomMarginRatio;
    this.leftMarginRatio = leftMarginRatio;
    this.rightMarginRatio = rightMarginRatio;
  }

  @Override
  public boolean perform(InputInjector injector, UiElement element) {
    Rect elementRect = element.getVisibleBounds();

    int topMargin = (int) (elementRect.height() * topMarginRatio);
    int bottomMargin = (int) (elementRect.height() * bottomMarginRatio);
    int leftMargin = (int) (elementRect.width() * leftMarginRatio);
    int rightMargin = (int) (elementRect.width() * rightMarginRatio);
    int adjustedbottom = elementRect.bottom - bottomMargin;
    int adjustedTop = elementRect.top + topMargin;
    int adjustedLeft = elementRect.left + leftMargin;
    int adjustedRight = elementRect.right - rightMargin;
    int startX;
    int startY;
    int endX;
    int endY;

    switch (direction) {
      case DOWN:
        startX = elementRect.centerX();
        startY = adjustedbottom;
        endX = elementRect.centerX();
        endY = adjustedTop;
        break;
      case UP:
        startX = elementRect.centerX();
        startY = adjustedTop;
        endX = elementRect.centerX();
        endY = adjustedbottom;
        break;
      case LEFT:
        startX = adjustedLeft;
        startY = elementRect.centerY();
        endX = adjustedRight;
        endY = elementRect.centerY();
        break;
      case RIGHT:
        startX = adjustedRight;
        startY = elementRect.centerY();
        endX = adjustedLeft;
        endY = elementRect.centerY();
        break;
      default:
        throw new ActionException("Unknown scroll direction: " + direction);
    }

    double xStep = ((double) (endX - startX)) / steps;
    double yStep = ((double) (endY - startY)) / steps;

    // First touch starts exactly at the point requested
    long downTime = Events.touchDown(injector, startX, startY);
    SystemClock.sleep(ACTION_MOVE_INTERVAL);
    if (drag) {
      SystemClock.sleep((long) (ViewConfiguration.getLongPressTimeout() * 1.5f));
    }
    for (int i = 1; i < steps; i++) {
      Events.touchMove(injector, downTime, startX + (int) (xStep * i), startY + (int) (yStep * i));
      SystemClock.sleep(ACTION_MOVE_INTERVAL);
    }
    if (drag) {
      // Hold final position for a little bit to simulate drag.
      SystemClock.sleep(100);
    }
    Events.touchUp(injector, downTime, endX, endY);
    return true;
  }

  @Override
  public String toString() {
    ToStringHelper toStringHelper = Strings.toStringHelper(this);
    toStringHelper.addValue(direction);
    toStringHelper.add("steps", steps);
    if (drag) {
      toStringHelper.addValue("drag");
    }
    return toStringHelper.toString();
  }
}
