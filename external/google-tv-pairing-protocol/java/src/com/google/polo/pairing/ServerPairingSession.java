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

package com.google.polo.pairing;

import com.google.polo.exception.NoConfigurationException;
import com.google.polo.exception.PoloException;
import com.google.polo.pairing.message.ConfigurationAckMessage;
import com.google.polo.pairing.message.ConfigurationMessage;
import com.google.polo.pairing.message.EncodingOption;
import com.google.polo.pairing.message.OptionsMessage.ProtocolRole;
import com.google.polo.pairing.message.PairingRequestAckMessage;
import com.google.polo.pairing.message.PairingRequestMessage;
import com.google.polo.pairing.message.PoloMessage.PoloMessageType;
import com.google.polo.wire.PoloWireInterface;

import java.io.IOException;

/**
 * Pairing session state machine implementation for a server.
 */
public class ServerPairingSession extends PairingSession {
  /**
   * Optional server name.
   */
  private final String mServerName;

  public ServerPairingSession(PoloWireInterface protocol,
      PairingContext context) {
    this(protocol, context, null);
  }

  public ServerPairingSession(PoloWireInterface protocol,
      PairingContext context, String serverName) {
    super(protocol, context);
    mServerName = serverName;
  }

  @Override
  protected void doInitializationPhase() throws PoloException, IOException {
    logDebug("Waiting for PairingRequest...");
    PairingRequestMessage request = (PairingRequestMessage)
        getNextMessage(PoloMessageType.PAIRING_REQUEST);

    logDebug("Requested service to pair: " + request.getServiceName());
    mServiceName = request.getServiceName();

    if (request.hasClientName()) {
      logDebug("Client name: " + request.getClientName());
      mPeerName = request.getClientName();
    } else {
      mPeerName = null;
    }

    logDebug("Sending PairingRequestAck ...");
    PairingRequestAckMessage ack = new PairingRequestAckMessage(mServerName);
    sendMessage(ack);

    logDebug("Waiting for Options ...");
    // Nothing to do with these; the client is responsible for intersecting
    // our options with its, and proposing a valid configuration.
    getNextMessage(PoloMessageType.OPTIONS);

    logDebug("Sending Options...");
    sendMessage(mLocalOptions);
  }

  @Override
  protected void doConfigurationPhase() throws PoloException, IOException {
    logDebug("Waiting for Configuration...");
    setConfiguration((ConfigurationMessage) getNextMessage(
        PoloMessageType.CONFIGURATION));

    // Verify the configuration
    EncodingOption encoding = mSessionConfig.getEncoding();

    if (getLocalRole() == ProtocolRole.DISPLAY_DEVICE) {
      if (!mLocalOptions.supportsOutputEncoding(encoding)) {
        throw new NoConfigurationException("Cannot support requested  " +
            "output encoding: " + encoding.getType());
      }
    } else if (getLocalRole() == ProtocolRole.INPUT_DEVICE) {
      if (!mLocalOptions.supportsInputEncoding(encoding)) {
        throw new NoConfigurationException("Cannot support requested " +
            "input encoding: " + encoding.getType());
      }
    } else {
      throw new IllegalStateException(); // should never happen
    }

    // Configuration accepted
    logDebug("Sending ConfigurationAck...");
    sendMessage(new ConfigurationAckMessage());
  }

  /**
   * Returns {@code true} if server name is set.
   */
  public boolean hasServerName() {
    return mServerName != null;
  }

  /**
   * Returns {@code true} if client name is set.
   */
  public boolean hasClientName() {
    return hasPeerName();
  }

  /**
   * Returns server name, or {@code null} if not set.
   */
  public String getServerName() {
    return mServerName;
  }

  /**
   * Returns client name, or {@code null} if not set.
   */
  public String getClientName() {
    return getPeerName();
  }
}
