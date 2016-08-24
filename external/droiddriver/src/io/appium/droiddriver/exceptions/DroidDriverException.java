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

/**
 * Base exception for DroidDriver.
 *
 * <p>All exceptions should extend this.
 */
@SuppressWarnings("serial")
public class DroidDriverException extends RuntimeException {
  public DroidDriverException(String message) {
    super(message);
  }

  public DroidDriverException(Throwable cause) {
    super(cause);
  }

  public DroidDriverException(String message, Throwable cause) {
    super(message, cause);
  }

  /**
   * Adapted from <a href="http://guava-libraries.googlecode.com">Guava libraries</a>. <p>
   * Propagates {@code throwable} as-is if it is an instance of {@link RuntimeException} or {@link
   * Error}, or else as a last resort, wraps it in a {@code DroidDriverException} and then
   * propagates. <p> This method always throws an exception. The {@code DroidDriverException} return
   * type is only for client code to make Java type system happy in case a return value is required
   * by the enclosing method. Example usage:
   * <pre>
   *   T doSomething() {
   *     try {
   *       return someMethodThatCouldThrowAnything();
   *     } catch (IKnowWhatToDoWithThisException e) {
   *       return handle(e);
   *     } catch (Throwable t) {
   *       throw DroidDriverException.propagate(t);
   *     }
   *   }
   * </pre>
   *
   * @param throwable the Throwable to propagate
   * @return nothing will ever be returned; this return type is only for your convenience, as
   * illustrated in the example above
   */
  public static DroidDriverException propagate(Throwable throwable) {
    if (throwable instanceof RuntimeException) {
      throw (RuntimeException) throwable;
    }
    if (throwable instanceof Error) {
      throw (Error) throwable;
    }
    throw new DroidDriverException(throwable);
  }
}
