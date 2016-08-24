/*
 * Copyright (C) 2009 Google Inc.  All rights reserved.
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

package com.google.polo.exception;

/**
 * Exception thrown when the secret given is incorrect.
 */
public class BadSecretException extends PoloException {

  public BadSecretException() {
    super();
  }

  public BadSecretException(String message, Throwable cause) {
    super(message, cause);
  }

  public BadSecretException(String message) {
    super(message);
  }

  public BadSecretException(Throwable cause) {
    super(cause);
  }

}
