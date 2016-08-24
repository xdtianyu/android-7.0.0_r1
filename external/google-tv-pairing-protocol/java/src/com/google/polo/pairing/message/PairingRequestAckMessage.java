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
public class PairingRequestAckMessage extends PoloMessage {

  /**
   * Server name.
   */
  private final String mServerName;

  public PairingRequestAckMessage() {
    this(null);
  }

  public PairingRequestAckMessage(String serverName) {
    super(PoloMessage.PoloMessageType.PAIRING_REQUEST_ACK);
    mServerName = serverName;
  }

  public String getServerName() {
    return mServerName;
  }

  public boolean hasServerName() {
    return mServerName != null;
  }

  @Override
  public String toString() {
    return new StringBuilder()
        .append("[")
        .append(getType())
        .append(" server_name=")
        .append(mServerName)
        .append("]").toString();
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }

    if (!(obj instanceof PairingRequestAckMessage)) {
      return false;
    }

    PairingRequestAckMessage other = (PairingRequestAckMessage) obj;

    if (mServerName == null) {
      if (other.mServerName != null) {
        return false;
      }
    } else if (!mServerName.equals(other.mServerName)) {
      return false;
    }
    return true;
  }

}
