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
 * Object implementing the internal representation of the protocol message
 * 'CONFIGURATION'.
 */
public class ConfigurationMessage extends PoloMessage {
  
  /**
   * The encoding component of the message.
   */
  private EncodingOption mEncoding;
  
  /**
   * The client role component of the message.
   */
  private OptionsMessage.ProtocolRole mClientRole;
  
  public ConfigurationMessage(EncodingOption enc,
      OptionsMessage.ProtocolRole role) {
    super(PoloMessage.PoloMessageType.CONFIGURATION);
    mEncoding = enc;
    mClientRole = role;
  }
  
  public EncodingOption getEncoding() {
    return mEncoding;
  }
  
  public OptionsMessage.ProtocolRole getClientRole() {
    return mClientRole;
  }
  
  @Override
  public String toString() {
    StringBuilder ret = new StringBuilder();
    ret.append("[");
    ret.append(getType());
    ret.append(" encoding=");
    ret.append(mEncoding);
    ret.append(", client_role=");
    ret.append(mClientRole);
    ret.append("]");
    return ret.toString();
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }

    if (!(obj instanceof ConfigurationMessage)) {
      return false;
    }

    ConfigurationMessage other = (ConfigurationMessage) obj;
    
    if (mEncoding == null) {
      if (other.mEncoding != null) {
        return false;
      }
    } else if (!mEncoding.equals(other.mEncoding)) {
      return false;
    }
    
    if (mClientRole == null) {
      if (other.mClientRole != null) {
        return false;
      }
    } else if (!mClientRole.equals(other.mClientRole)) {
      return false;
    }
    
    return true;
  }
  
}
