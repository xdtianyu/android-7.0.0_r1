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
import io.appium.droiddriver.scroll.Direction.PhysicalDirection;

/**
 * Interface for performing actions on a {@link UiElement}.
 */
public interface UiElementActor {
  /**
   * Clicks this element. The click will be at the center of the visible
   * element.
   */
  void click(UiElement uiElement);

  /**
   * Long-clicks this element. The click will be at the center of the visible
   * element.
   */
  void longClick(UiElement uiElement);

  /**
   * Double-clicks this element. The click will be at the center of the visible
   * element.
   */
  void doubleClick(UiElement uiElement);

  /**
   * Scrolls in the given direction.
   *
   * @param direction specifies where the view port will move, instead of the
   *        finger.
   */
  void scroll(UiElement uiElement, PhysicalDirection direction);
}
