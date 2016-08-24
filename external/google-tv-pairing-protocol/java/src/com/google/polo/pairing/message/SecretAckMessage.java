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

import com.google.polo.pairing.PoloUtil;

import java.util.Arrays;

/**
 * Object implementing the internal representation of the protocol message
 * 'SECRET_ACK'.
 */
public class SecretAckMessage extends PoloMessage {
  
  private byte[] mSecret;
  
  public SecretAckMessage(byte[] secret) {
    super(PoloMessage.PoloMessageType.SECRET_ACK);
    mSecret = secret;
  }
  
  public byte[] getSecret() {
    return mSecret;
  }

  @Override
  public String toString() {
    StringBuilder ret = new StringBuilder();
    ret.append("[");
    ret.append(getType());
    ret.append(" secret=");
    ret.append(PoloUtil.bytesToHexString(mSecret));
    ret.append("]");
    return ret.toString();
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }

    if (!(obj instanceof SecretAckMessage)) {
      return false;
    }
    
    SecretAckMessage other = (SecretAckMessage) obj;
    return Arrays.equals(mSecret, other.mSecret);
  }
  
}
