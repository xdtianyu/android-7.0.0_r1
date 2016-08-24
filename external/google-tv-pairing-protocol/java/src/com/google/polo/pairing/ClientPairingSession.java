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

import com.google.polo.exception.PoloException;
import com.google.polo.pairing.message.ConfigurationAckMessage;
import com.google.polo.pairing.message.OptionsMessage;
import com.google.polo.pairing.message.PairingRequestAckMessage;
import com.google.polo.pairing.message.PairingRequestMessage;
import com.google.polo.pairing.message.PoloMessage.PoloMessageType;
import com.google.polo.wire.PoloWireInterface;

import java.io.IOException;

/**
 * Pairing session implementation for a client.
 */
public class ClientPairingSession extends PairingSession {

  /**
   * Client name;
   */
  private final String mClientName;

  /**
   * Constructor.
   *
   * @param protocol     the wire interface for the session
   * @param context      the pairing context for the session
   * @param serviceName  the string service name, used in the pairing request
   */
  public ClientPairingSession(PoloWireInterface protocol,
      PairingContext context, String serviceName) {
    this(protocol, context, serviceName, null);
  }

  /**
   * Constructor.
   *
   * @param protocol     the wire interface for the session
   * @param context      the pairing context for the session
   * @param serviceName  the string service name, used in the pairing request
   * @param clientName   the string client name, used in the pairing request
   */
  public ClientPairingSession(PoloWireInterface protocol,
      PairingContext context, String serviceName, String clientName) {
    super(protocol, context);
    mServiceName = serviceName;
    mClientName = clientName;
  }

  @Override
  protected void doInitializationPhase()
      throws PoloException, IOException {
    logDebug("Sending PairingRequest... " + mServiceName + " " + mClientName);
    PairingRequestMessage msg = new PairingRequestMessage(mServiceName, mClientName);
    sendMessage(msg);

    logDebug("Waiting for PairingRequestAck ...");
    PairingRequestAckMessage ack = (PairingRequestAckMessage) getNextMessage(
        PoloMessageType.PAIRING_REQUEST_ACK);

    if (ack.hasServerName()) {
      mPeerName = ack.getServerName();
      logDebug("Got PairingRequestAck with server name = " + mPeerName);
    } else {
      mPeerName = null;
    }

    logDebug("Sending Options ...");
    sendMessage(mLocalOptions);

    logDebug("Waiting for Options...");
    OptionsMessage serverOptions = (OptionsMessage) getNextMessage(
        PoloMessageType.OPTIONS);

    // Compare compatibility with server options, and save config.
    logDebug("Local config = " + mLocalOptions);
    logDebug("Server options = " + serverOptions);
    setConfiguration(mLocalOptions.getBestConfiguration(serverOptions));
  }

  @Override
  protected void doConfigurationPhase() throws PoloException, IOException {
    logDebug("Sending Configuration...");
    sendMessage(mSessionConfig);
    logDebug("Waiting for ConfigurationAck...");
    ConfigurationAckMessage ack = (ConfigurationAckMessage)
        getNextMessage(PoloMessageType.CONFIGURATION_ACK);
  }

  /**
   * Returns {@code true} if client name is set.
   */
  public boolean hasClientName() {
    return mClientName != null;
  }

  /**
   * Returns {@code true} if server name is set.
   */
  public boolean hasServerName() {
    return hasPeerName();
  }

  /**
   * Returns client name, or {@code null} if not set.
   */
  public String getClientName() {
    return mClientName;
  }

  /**
   * Returns server name, or {@code null} if not set.
   */
  public String getServerName() {
    return getPeerName();
  }
}
