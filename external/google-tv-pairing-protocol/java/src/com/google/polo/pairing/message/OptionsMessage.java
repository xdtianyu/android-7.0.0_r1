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

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

/**
 * Object implementing the internal representation of the protocol message
 * 'OPTIONS'.
 */
public class OptionsMessage extends PoloMessage {
  
  /**
   * Representation of a specific role type. The numeric values
   * should be kept synchronized with the constants defined by
   * Options.Encoding.EncodingType in polo.proto. 
   */
  public static enum ProtocolRole {
    UNKNOWN(0),
    INPUT_DEVICE(1),
    DISPLAY_DEVICE(2);
    
    private final int mIntVal;
    
    private ProtocolRole(int intVal) {
      mIntVal = intVal;
    }
    
    public static ProtocolRole fromIntVal(int intVal) {
      for (ProtocolRole role : ProtocolRole.values()) {
        if (role.getAsInt() == intVal) {
          return role;
        }
      }
      return ProtocolRole.UNKNOWN;
    }
    
    public int getAsInt() {
      return mIntVal;
    }
  }
      
  /**
   * The preferred protocol role of the sender.
   */
  private ProtocolRole mProtocolRolePreference;
  
  /**
   * Sender's supported input encodings.
   */
  private Set<EncodingOption> mInputEncodings;
  
  /**
   * Sender's supported output encodings.
   */
  private Set<EncodingOption> mOutputEncodings;
  
  public OptionsMessage() {
    super(PoloMessage.PoloMessageType.OPTIONS);
    
    mProtocolRolePreference = ProtocolRole.UNKNOWN;
    mInputEncodings = new HashSet<EncodingOption>();
    mOutputEncodings = new HashSet<EncodingOption>();
  }
  
  public void setProtocolRolePreference(ProtocolRole pref) {
    mProtocolRolePreference = pref;
  }
  
  public ProtocolRole getProtocolRolePreference() {
    return mProtocolRolePreference;
  }
  
  public void addInputEncoding(EncodingOption encoding) {
    mInputEncodings.add(encoding);
  }  
  
  public void addOutputEncoding(EncodingOption encoding) {
    mOutputEncodings.add(encoding);
  }
  
  public boolean supportsInputEncoding(EncodingOption encoding) {
    return mInputEncodings.contains(encoding);
  }  
  
  public boolean supportsOutputEncoding(EncodingOption encoding) {
    return mOutputEncodings.contains(encoding);
  }
  
  public Set<EncodingOption> getInputEncodingSet() {
    return new HashSet<EncodingOption>(mInputEncodings);
  }
  
  public Set<EncodingOption> getOutputEncodingSet() {
    return new HashSet<EncodingOption>(mOutputEncodings);
  }
  
  /**
   * Generates a ConfigurationMessage, or {@code null}, by intersecting the
   * local message with another message. 
   */
  public ConfigurationMessage getBestConfiguration(
      OptionsMessage otherOptions) {
    Set<EncodingOption> peerOutputs = otherOptions.getOutputEncodingSet();
    peerOutputs.retainAll(mInputEncodings);
    
    Set<EncodingOption> peerInputs = otherOptions.getInputEncodingSet();
    peerInputs.retainAll(mOutputEncodings);
    
    if (peerOutputs.isEmpty() && peerInputs.isEmpty()) {
      // No configuration possible: no common encodings.
      return null;
    }
    
    EncodingOption encoding;
    ProtocolRole role;
    
    EncodingOption bestInput = null;
    if (!peerInputs.isEmpty()) {
      bestInput = peerInputs.iterator().next();
    }
    EncodingOption bestOutput = null;
    if (!peerOutputs.isEmpty()) {
      bestOutput = peerOutputs.iterator().next();
    }

    if (mProtocolRolePreference == ProtocolRole.DISPLAY_DEVICE) {
      // We prefer to be the display device
      if (bestInput != null) {
        encoding = bestInput;
        role = ProtocolRole.DISPLAY_DEVICE;
      } else {
        encoding = bestOutput;
        role = ProtocolRole.INPUT_DEVICE;
      }
    } else {
      // We prefer to be the input device, or have no preference
      if (!peerOutputs.isEmpty()) {
        encoding = bestOutput;
        role = ProtocolRole.INPUT_DEVICE;
      } else {
        encoding = bestInput;
        role = ProtocolRole.DISPLAY_DEVICE;
      }
    }
    
    return new ConfigurationMessage(encoding, role);
  }
  
  @Override
  public String toString() {
    StringBuilder ret = new StringBuilder();
    ret.append("[" + getType() + " ");
    
    ret.append("inputs=");
    Iterator<EncodingOption> inputIterator = mInputEncodings.iterator();
    while (inputIterator.hasNext()) {
      ret.append(inputIterator.next());
      if (inputIterator.hasNext()) {
        ret.append(",");
      }
    }
    
    ret.append(" outputs=");
    Iterator<EncodingOption> outputIterator = mOutputEncodings.iterator();
    while (outputIterator.hasNext()) {
      ret.append(outputIterator.next());
      if (outputIterator.hasNext()) {
        ret.append(",");
      }
    }
    
    ret.append(" pref=" + mProtocolRolePreference);
    ret.append("]");
    return ret.toString();
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }

    if (!(obj instanceof OptionsMessage)) {
      return false;
    }

    OptionsMessage other = (OptionsMessage) obj;
    
    if (mProtocolRolePreference == null) {
      if (other.mProtocolRolePreference != null) {
        return false;
      }
    } else if (!mProtocolRolePreference.equals(other.mProtocolRolePreference)) {
        return false;
    }

    return mInputEncodings.equals(other.mInputEncodings) &&
        mOutputEncodings.equals(other.mOutputEncodings);
  }
  
}
