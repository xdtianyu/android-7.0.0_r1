/*
 * Copyright (C) 2015 DroidDriver committers
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

package io.appium.droiddriver.helpers;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Base class for an action that should run only once no matter how many times the method {@link
 * #singleRun()} is called upon an instance. Typically it is used on a singleton to achieve once for
 * a class effect.
 */
public abstract class SingleRun {
  private AtomicBoolean hasRun = new AtomicBoolean();

  /**
   * Calls {@link #run()} if it is the first time this method is called upon this instance.
   *
   * @return true if this is the first time it is called, otherwise false
   */
  public final boolean singleRun() {
    if (hasRun.compareAndSet(false, true)) {
      run();
      return true;
    }
    return false;
  }

  /**
   * Takes the action that should run only once.
   */
  protected abstract void run();
}
