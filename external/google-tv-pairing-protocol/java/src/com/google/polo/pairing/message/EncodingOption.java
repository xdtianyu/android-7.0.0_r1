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

package com.google.polo.pairing.message;

/**
 * Internal representation of a session encoding option.
 */
public class EncodingOption {

  /**
   * Representation of a specific encoding type. The numeric values
   * should be kept synchronized with the constants defined by
   * Options.Encoding.EncodingType in polo.proto. 
   */
  public enum EncodingType {
    ENCODING_UNKNOWN(0),
    ENCODING_ALPHANUMERIC(1),
    ENCODING_NUMERIC(2),
    ENCODING_HEXADECIMAL(3),
    ENCODING_QRCODE(4);
    
    private final int mIntVal;
    
    private EncodingType(int intVal) {
      mIntVal = intVal;
    }
    
    public static EncodingType fromIntVal(int intVal) {
      for (EncodingType encType : EncodingType.values()) {
        if (encType.getAsInt() == intVal) {
          return encType;
        }
      }
      return EncodingType.ENCODING_UNKNOWN;
    }
    
    public int getAsInt() {
      return mIntVal;
    }
  }

  private EncodingType mType;
  
  private int mSymbolLength;
  
  public EncodingOption(EncodingType type, int symbolLength) {
    mType = type;
    mSymbolLength = symbolLength;
  }
  
  @Override
  public String toString() {
    return mType + ":" + mSymbolLength;
  }
  
  public EncodingType getType() {
    return mType;
  }

  public int getSymbolLength() {
    return mSymbolLength;
  }
  
  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }

    if (!(obj instanceof EncodingOption)) {
      return false;
    }

    EncodingOption other = (EncodingOption) obj;
    if (mType == null) {
      if (other.mType != null) {
        return false;
      }
    } else if (!mType.equals(other.mType)) {
      return false;
    }

    return mSymbolLength == other.mSymbolLength;
  }
  
  @Override
  public int hashCode() {
    int result = 7;
    result = result * 31 + (mType != null ? mType.hashCode() : 0);
    result = result * 31 + mSymbolLength;
    return result;
  }

}
