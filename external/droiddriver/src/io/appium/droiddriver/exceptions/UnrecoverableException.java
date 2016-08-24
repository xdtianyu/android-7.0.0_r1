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

package io.appium.droiddriver.exceptions;

import io.appium.droiddriver.helpers.BaseDroidDriverTest;

/**
 * When an {@link UnrecoverableException} occurs, the rest of the tests are
 * going to fail as well, therefore running them only adds noise to the report.
 * {@link BaseDroidDriverTest} will skip remaining tests when this is thrown.
 */
@SuppressWarnings("serial")
public class UnrecoverableException extends RuntimeException {
  public UnrecoverableException(String message) {
    super(message);
  }

  public UnrecoverableException(Throwable throwable) {
    super(throwable);
  }
}
