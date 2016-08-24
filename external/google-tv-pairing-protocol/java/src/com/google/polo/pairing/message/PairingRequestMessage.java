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
 * 'PAIRING_REQUEST'.
 */
public class PairingRequestMessage extends PoloMessage {
  
  /**
   * The service name being requested for pairing.
   */
  private final String mServiceName;

  /**
   * Client name.
   */
  private final String mClientName;

  public PairingRequestMessage(String serviceName) {
    this(serviceName, null);
  }

  public PairingRequestMessage(String serviceName, String clientName) {
    super(PoloMessage.PoloMessageType.PAIRING_REQUEST);
    mServiceName = serviceName;
    mClientName = clientName;
  }

  public String getServiceName() {
    return mServiceName;
  }

  public String getClientName() {
    return mClientName;
  }

  public boolean hasClientName() {
    return mClientName != null;
  }

  @Override
  public String toString() {
    return new StringBuilder()
        .append("[")
        .append(getType())
        .append(" service_name=")
        .append(mServiceName)
        .append(", client_name=")
        .append(mClientName)
        .append("]").toString();
  }
  
  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }

    if (!(obj instanceof PairingRequestMessage)) {
      return false;
    }
    
    PairingRequestMessage other = (PairingRequestMessage) obj;
    
    if (mServiceName == null) {
      if (other.mServiceName != null) {
        return false;
      }
    } else if (!mServiceName.equals(other.mServiceName)) {
      return false;
    } else if (mClientName == null) {
      if (other.mClientName != null) {
        return false;
      }
    } else if (!mClientName.equals(other.mClientName)) {
      return false;
    }

    return true;
  }
  
}
