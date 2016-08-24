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

import com.google.polo.pairing.PoloUtil;

/**
 * A {@link SecretEncoder} for the hexadecimal secret encoding scheme.
 */
public class HexadecimalEncoder implements SecretEncoder {

  public static final int SYMBOLS_PER_BYTE = 2;

  public byte[] decodeToBytes(String encodedString) {
    return PoloUtil.hexStringToBytes(encodedString);
  }

  public String encodeToString(byte[] secretBytes) {
    return PoloUtil.bytesToHexString(secretBytes).toLowerCase();
  }

  public int symbolsPerByte() {
    return SYMBOLS_PER_BYTE;
  }

}
