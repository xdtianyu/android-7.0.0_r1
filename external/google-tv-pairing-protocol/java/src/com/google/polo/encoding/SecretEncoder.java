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

package com.google.polo.encoding;

/**
 * Methods that must be implemented by a secret encoding scheme.
 */
public interface SecretEncoder {

  /**
   * Converts the byte representation of a secret to the appropriate string
   * value in this encoding.
   */
  public String encodeToString(byte[] secretBytes);
  
  /**
   * Converts the encoded value of a secret to the raw byte representation.
   */
  public byte[] decodeToBytes(String encodedString);

  /**
   * Returns the number of symbols produced per each byte of raw input.
   */
  public int symbolsPerByte();

}
