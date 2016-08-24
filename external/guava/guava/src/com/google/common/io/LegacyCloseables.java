/*
 * Copyright (C) 2007 The Guava Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.common.io;

import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.io.Reader;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.annotation.Nullable;

/**
 * Legacy methods for working with {@link Closeable} objects.
 */
public final class LegacyCloseables {
  private static final Logger logger = Logger.getLogger(LegacyCloseables.class.getName());

  private LegacyCloseables() {}

  /**
   * Closes a {@link Closeable}, if an IOException is thrown by {@link Closeable#close()} then it is
   * caught and logged.
   *
   * This is primarily useful in a finally block, where a thrown exception needs to be logged but
   * not propagated (otherwise the original exception will be lost). If possible, use
   * try-with-resources instead as that captures all the exceptions. Use this only if that is not
   * possible, either because the application has to work on Android at an API level lower than 19
   * that does not support try-with-resources, or the exceptions truly can be ignored.
   *
   * @param closeable the {@code Closeable} object to be closed, or null, in which case this method
   *     does nothing
   * @deprecated This method may suppress potentially significant exceptions, particularly when
   *     closing writable resources. With a writable resource, a failure thrown from {@code close()}
   *     should be considered as significant as a failure thrown from a write method because it may
   *     indicate a failure to flush bytes to the underlying resource. When possible, use the
   *     Source/Sink classes to handle opening and closing resources for you. When that isn't
   *     possible, use
   *     <a href="http://docs.oracle.com/javase/tutorial/essential/exceptions/tryResourceClose.html">
   *     try-with-resources</a>. When that isn't possible and you are closing a readable
   *     resource, use {@link #closeQuietly(InputStream)} or {@link #closeQuietly(Reader)}. See
   *     <a href="https://code.google.com/p/guava-libraries/wiki/ClosingResourcesExplained">this
   *     Guava wiki article</a> for more information on the problem with writable resources and the
   *     alternatives to this method.
   */
  @Deprecated
  public static void closeQuietly(@Nullable Closeable closeable) {
    if (closeable == null) {
      return;
    }
    try {
      closeable.close();
    } catch (IOException e) {
      logger.log(Level.WARNING,
          "IOException thrown while closing Closeable.", e);
    }
  }
}
