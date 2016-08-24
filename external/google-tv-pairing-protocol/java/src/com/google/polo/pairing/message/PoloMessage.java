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

import com.google.polo.wire.PoloWireInterface;

/**
 * Base class for internal representation of Polo protocol messages.
 * 
 * <p>Implementations of {@link PoloWireInterface} will translate to and from
 * the wire version of protocol messages to subclasses of PoloMessage.
 */
public class PoloMessage {
  
  /**
   * List of all available Polo protocol message types, in order of protocol
   * phases.  The values are synchronized with the constants defined in
   * polo.proto.
   */
  public static enum PoloMessageType {
    UNKNOWN(0),
    PAIRING_REQUEST(10),
    PAIRING_REQUEST_ACK(11),
    OPTIONS(20),
    CONFIGURATION(30),
    CONFIGURATION_ACK(31),
    SECRET(40),
    SECRET_ACK(41);
    
    private final int mIntVal;
    
    private PoloMessageType(int intVal) {
      mIntVal = intVal;
    }
    
    public static PoloMessageType fromIntVal(int intVal) {
      for (PoloMessageType messageType : PoloMessageType.values()) {
        if (messageType.getAsInt() == intVal) {
          return messageType;
        }
      }
      return null;
    }
    
    public int getAsInt() {
      return mIntVal;
    }
  }
    
  /**
   * The Polo protocol message type of this instance.
   */
  private final PoloMessageType mType;
  
  public PoloMessage(PoloMessageType type) {
    mType = type;
  }
  
  public PoloMessageType getType() {
    return mType;
  }

  @Override
  public String toString() {
    return "[" + mType.toString() + "]";
  }
  
}
