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

import io.appium.droiddriver.DroidDriver;
import io.appium.droiddriver.exceptions.UnrecoverableException;

/**
 * Calls {@link DroidDrivers#setSingleton} once and only once.
 */
public class DroidDriversInitializer extends SingleRun {
  private static DroidDriversInitializer instance;
  private final DroidDriver driver;

  private DroidDriversInitializer(DroidDriver driver) {
    this.driver = driver;
  }

  @Override
  protected void run() {
    DroidDrivers.setSingleton(driver);
  }

  public static synchronized DroidDriversInitializer get(DroidDriver driver) {
    if (instance == null) {
      instance = new DroidDriversInitializer(driver);
    }

    if (instance.driver != driver) {
      throw new UnrecoverableException("The singleton DroidDriversInitializer has already been" +
          " created with a different driver");
    }
    return instance;
  }
}
