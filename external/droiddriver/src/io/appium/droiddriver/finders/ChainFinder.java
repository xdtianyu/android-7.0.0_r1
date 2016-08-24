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

package io.appium.droiddriver.finders;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.util.Preconditions;

/**
 * Finds UiElement by applying Finders in turn: using the UiElement returned by
 * first Finder as context for the second Finder. It is conceptually similar to
 * <a href="http://en.wikipedia.org/wiki/Functional_composition">Function
 * composition</a>. The returned UiElement can be thought of as the result of
 * second(first(context)).
 * <p>
 * Note typically first Finder finds the ancestor, then second Finder finds the
 * target UiElement, which is a descendant. ChainFinder can be chained with
 * additional Finders to make a "chain".
 */
public class ChainFinder implements Finder {
  private final Finder first;
  private final Finder second;

  protected ChainFinder(Finder first, Finder second) {
    this.first = Preconditions.checkNotNull(first);
    this.second = Preconditions.checkNotNull(second);
  }

  @Override
  public String toString() {
    return String.format("Chain{%s, %s}", first, second);
  }

  @Override
  public UiElement find(UiElement context) {
    return second.find(first.find(context));
  }
}
