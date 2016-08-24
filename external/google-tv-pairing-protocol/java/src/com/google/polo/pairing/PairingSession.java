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

import com.google.polo.encoding.HexadecimalEncoder;
import com.google.polo.encoding.SecretEncoder;
import com.google.polo.exception.BadSecretException;
import com.google.polo.exception.NoConfigurationException;
import com.google.polo.exception.PoloException;
import com.google.polo.exception.ProtocolErrorException;
import com.google.polo.pairing.PairingListener.LogLevel;
import com.google.polo.pairing.message.ConfigurationMessage;
import com.google.polo.pairing.message.EncodingOption;
import com.google.polo.pairing.message.OptionsMessage;
import com.google.polo.pairing.message.OptionsMessage.ProtocolRole;
import com.google.polo.pairing.message.PoloMessage;
import com.google.polo.pairing.message.PoloMessage.PoloMessageType;
import com.google.polo.pairing.message.SecretAckMessage;
import com.google.polo.pairing.message.SecretMessage;
import com.google.polo.wire.PoloWireInterface;

import java.io.IOException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.cert.Certificate;
import java.util.Arrays;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;


/**
 * Implements the logic of and holds state for a single occurrence of the
 * pairing protocol.
 * <p>
 * This abstract class implements the logic common to both client and server
 * perspectives of the protocol.  Notably, the 'pairing' phase of the
 * protocol has the same logic regardless of client/server status
 * ({link PairingSession#doPairingPhase()}). Other phases of the protocol are
 * specific to client/server status; see {@link ServerPairingSession} and
 * {@link ClientPairingSession}.
 * <p>
 * The protocol is initiated by called
 * {@link PairingSession#doPair(PairingListener)}
 * The listener implementation is responsible for showing the shared secret
 * to the user
 * ({@link PairingListener#onPerformOutputDeviceRole(PairingSession, byte[])}),
 * or in accepting the user input
 * ({@link PairingListener#onPerformInputDeviceRole(PairingSession)}),
 * depending on the role negotiated during initialization.
 * <p>
 * When operating in the input role, the session will block execution after
 * calling {@link PairingListener#onPerformInputDeviceRole(PairingSession)} to
 * wait for the secret.  The listener, or some activity resulting from it, must
 * publish the input secret to the session via
 * {@link PairingSession#setSecret(byte[])}.
 */
public abstract class PairingSession {

  protected enum ProtocolState {
      STATE_UNINITIALIZED,
      STATE_INITIALIZING,
      STATE_CONFIGURING,
      STATE_PAIRING,
      STATE_SUCCESS,
      STATE_FAILURE,
  }

  /**
   * Enable extra verbose debug logging.
   */
  private static final boolean DEBUG_VERBOSE = false;

  /**
   * Controls whether to verify the secret portion of the SecretAck message.
   * <p>
   * NOTE(mikey): One implementation does not send the secret back in
   * the SecretAck.  This should be fixed, but in the meantime it is not
   * essential that we verify it, since *any* acknowledgment from the
   * sender is enough to indicate protocol success.
   */
  private static final boolean VERIFY_SECRET_ACK = false;

  /**
   * Timeout, in milliseconds, for polling the secret queue for a response from
   * the listener.  This timeout is relevant only to periodically check the
   * mAbort flag to terminate the protocol, which is set by calling teardown().
   */
  private static final int SECRET_POLL_TIMEOUT_MS = 500;

  /**
   * Performs the initialization phase of the protocol.
   *
   * @throws PoloException  if a protocol error occurred
   * @throws IOException    if an error occurred in input/output
   */
  protected abstract void doInitializationPhase()
      throws PoloException, IOException;

  /**
   * Performs the configuration phase of the protocol.
   *
   * @throws PoloException  if a protocol error occurred
   * @throws IOException    if an error occurred in input/output
   */
  protected abstract void doConfigurationPhase()
      throws PoloException, IOException;

  /**
   * Internal representation of challenge-response.
   */
  protected PoloChallengeResponse mChallenge;

  /**
   * Implementation of the transport layer.
   */
  private final PoloWireInterface mProtocol;

  /**
   * Context for the pairing session.
   */
  protected final PairingContext mPairingContext;

  /**
   * Local endpoint's supported options.
   * <p>
   * If this session is acting as a server, this message will be sent to the
   * client in the Initialization phase.  If acting as a client, this member is
   * used to store local options and compute the Configuration message (but
   * is never transmitted directly).
   */
  protected OptionsMessage mLocalOptions;

  /**
   * Encoding scheme used for the session.
   */
  protected SecretEncoder mEncoder;

  /**
   * Name of the service being paired.
   */
  protected String mServiceName;

  /**
   * Name of the peer.
   */
  protected String mPeerName;

  /**
   * Configuration message for current session.
   * <p>
   * This is computed by the client and sent to the server.
   */
  protected ConfigurationMessage mSessionConfig;

  /**
   * Listener that will receive callbacks upon protocol events.
   */
  protected PairingListener mListener;

  /**
   * Internal state of the pairing session.
   */
  protected ProtocolState mState;

  /**
   * Threadsafe queue for receiving the messages sent by peer, user-given secret
   * from the listener, or exceptions caught by async threads.
   */
  protected BlockingQueue<QueueMessage> mMessageQueue;

  /**
   * Flag set when the session should be aborted.
   */
  protected boolean mAbort;

  /**
   * Reader thread.
   */
  private final Thread mThread;

  /**
   * Constructor.
   *
   * @param protocol        the wire interface to operate against
   * @param pairingContext  a PairingContext for the session
   */
  public PairingSession(PoloWireInterface protocol,
      PairingContext pairingContext) {
    mProtocol = protocol;
    mPairingContext = pairingContext;
    mState = ProtocolState.STATE_UNINITIALIZED;
    mMessageQueue = new LinkedBlockingQueue<QueueMessage>();

    Certificate clientCert = mPairingContext.getClientCertificate();
    Certificate serverCert = mPairingContext.getServerCertificate();

    mChallenge = new PoloChallengeResponse(clientCert, serverCert,
        new PoloChallengeResponse.DebugLogger() {
          public void debug(String message) {
            logDebug(message);
          }
          public void verbose(String message) {
            if (DEBUG_VERBOSE) {
              logDebug(message);
            }
          }
        });

    mLocalOptions = new OptionsMessage();

    if (mPairingContext.isServer()) {
      mLocalOptions.setProtocolRolePreference(ProtocolRole.DISPLAY_DEVICE);
    } else {
      mLocalOptions.setProtocolRolePreference(ProtocolRole.INPUT_DEVICE);
    }

    mThread = new Thread(new Runnable() {
      public void run() {
        logDebug("Starting reader");
        try {
          while (!mAbort) {
            try {
              PoloMessage message = mProtocol.getNextMessage();
              logDebug("Received: " + message.getClass());
              mMessageQueue.put(new QueueMessage(message));
            } catch (PoloException exception) {
              logDebug("Exception while getting message: " + exception);
              mMessageQueue.put(new QueueMessage(exception));
              break;
            } catch (IOException exception) {
              logDebug("Exception while getting message: " + exception);
              mMessageQueue.put(new QueueMessage(new PoloException(exception)));
              break;
            }
          }
        } catch (InterruptedException ie) {
          logDebug("Interrupted: " + ie);
        } finally {
          logDebug("Reader is done");
        }
      }
    });
    mThread.start();
  }

  public void teardown() {
    try {
      // Send any error.
      mProtocol.sendErrorMessage(new Exception());
      mPairingContext.getPeerInputStream().close();
      mPairingContext.getPeerOutputStream().close();
    } catch (IOException e) {
      // oh well.
    }

    // Unblock the blocking wait on the secret queue.
    mAbort = true;
    mThread.interrupt();
  }

  protected void log(LogLevel level, String message) {
    if (mListener != null) {
      mListener.onLogMessage(level, message);
    }
  }

  /**
   * Logs a debug message to the active listener.
   */
  public void logDebug(String message) {
    log(LogLevel.LOG_DEBUG, message);
  }

  /**
   * Logs an informational message to the active listener.
   */
  public void logInfo(String message) {
    log(LogLevel.LOG_INFO, message);
  }

  /**
   * Logs an error message to the active listener.
   */
  public void logError(String message) {
    log(LogLevel.LOG_ERROR, message);
  }

  /**
   * Adds an encoding to the supported input role encodings.  This method can
   * only be called before the session has started.
   * <p>
   * If no input encodings have been added, then this endpoint cannot act as
   * the input device protocol role.
   *
   * @param encoding  the {@link EncodingOption} to add
   */
  public void addInputEncoding(EncodingOption encoding) {
    if (mState != ProtocolState.STATE_UNINITIALIZED) {
      throw new IllegalStateException("Cannot add encodings once session " +
          "has been started.");
    }
    // Legal values of GAMMALEN must be:
    // - an even number of bytes
    // - at least 2 bytes
    if ((encoding.getSymbolLength() < 2) ||
        ((encoding.getSymbolLength() % 2) != 0)) {
        throw new IllegalArgumentException("Bad symbol length: " +
            encoding.getSymbolLength());
    }
      mLocalOptions.addInputEncoding(encoding);
  }

  /**
   * Adds an encoding to the supported output role encodings.  This method can
   * only be called before the session has started.
   * <p>
   * If no output encodings have been added, then this endpoint cannot act as
   * the output device protocol role.
   *
   * @param encoding  the {@link EncodingOption} to add
   */
  public void addOutputEncoding(EncodingOption encoding) {
    if (mState != ProtocolState.STATE_UNINITIALIZED) {
      throw new IllegalStateException("Cannot add encodings once session " +
          "has been started.");
    }
    mLocalOptions.addOutputEncoding(encoding);
  }

  /**
   * Changes the internal state.
   *
   * @param newState  the new state
   */
  private void setState(ProtocolState newState) {
    logInfo("New state: " + newState);
    mState = newState;
  }

  /**
   * Runs the pairing protocol.
   * <p>
   * Supported input and output encodings must be specified
   * first, using
   * {@link PairingSession#addInputEncoding(EncodingOption)} and
   * {@link PairingSession#addOutputEncoding(EncodingOption)},
   * respectively.
   *
   * @param listener  the {@link PairingListener} for the session
   * @return {@code true} if pairing was successful
   */
  public boolean doPair(PairingListener listener) {
    mListener = listener;
    mListener.onSessionCreated(this);

    if (mPairingContext.isServer()) {
      logDebug("Protocol started (SERVER mode)");
    } else {
      logDebug("Protocol started (CLIENT mode)");
    }

    logDebug("Local options: " + mLocalOptions.toString());

    Certificate clientCert = mPairingContext.getClientCertificate();
    if (DEBUG_VERBOSE) {
      logDebug("Client certificate:");
      logDebug(clientCert.toString());
    }

    Certificate serverCert = mPairingContext.getServerCertificate();

    if (DEBUG_VERBOSE) {
      logDebug("Server certificate:");
      logDebug(serverCert.toString());
    }

    boolean success = false;

    try {
      setState(ProtocolState.STATE_INITIALIZING);
      doInitializationPhase();

      setState(ProtocolState.STATE_CONFIGURING);
      doConfigurationPhase();

      setState(ProtocolState.STATE_PAIRING);
      doPairingPhase();

      success = true;
    } catch (ProtocolErrorException e) {
      logDebug("Remote protocol failure: " + e);
    } catch (PoloException e) {
      try {
        logDebug("Local protocol failure, attempting to send error: " + e);
        mProtocol.sendErrorMessage(e);
      } catch (IOException e1) {
        logDebug("Error message send failed");
      }
    } catch (IOException e) {
      logDebug("IOException: " + e);
    }

    if (success) {
      setState(ProtocolState.STATE_SUCCESS);
    } else {
      setState(ProtocolState.STATE_FAILURE);
    }

    mListener.onSessionEnded(this);
    return success;
  }

  /**
   * Returns {@code true} if the session is in a terminal state (success or
   * failure).
   */
  public boolean hasCompleted() {
    switch (mState) {
      case STATE_SUCCESS:
      case STATE_FAILURE:
        return true;
      default:
        return false;
    }
  }

  public boolean hasSucceeded() {
    return mState == ProtocolState.STATE_SUCCESS;
  }

  public String getServiceName() {
    return mServiceName;
  }

  /**
   * Sets the secret, as received from a user.  This method is only meaningful
   * when the endpoint is acting as the input device role.
   *
   * @param secret  the secret, as a byte sequence
   * @return        {@code true} if the secret was captured
   */
  public boolean setSecret(byte[] secret) {
    if (!isInputDevice()) {
      throw new IllegalStateException("Secret can only be set for " +
          "input role session.");
    } else if (mState != ProtocolState.STATE_PAIRING) {
      throw new IllegalStateException("Secret can only be set while " +
          "in pairing state.");
    }
    return mMessageQueue.offer(new QueueMessage(secret));
  }

  /**
   * Executes the pairing phase of the protocol.
   *
   * @throws PoloException  if a protocol error occurred
   * @throws IOException    if an error in the input/output occurred
   */
  protected void doPairingPhase() throws PoloException, IOException {
    if (isInputDevice()) {
      new Thread(new Runnable() {
        public void run() {
          logDebug("Calling listener for user input...");
          try {
            mListener.onPerformInputDeviceRole(PairingSession.this);
          } catch (PoloException exception) {
            logDebug("Sending exception: " + exception);
            mMessageQueue.offer(new QueueMessage(exception));
          } finally {
            logDebug("Listener finished.");
          }
        }
      }).start();

      logDebug("Waiting for secret from Listener or ...");
      QueueMessage message = waitForMessage();
      if (message == null || !message.hasSecret()) {
        throw new PoloException(
            "Illegal state - no secret available: " + message);
      }
      byte[] userGamma = message.mSecret;
      if (userGamma == null) {
        throw new PoloException("Invalid secret.");
      }

      boolean match = mChallenge.checkGamma(userGamma);
      if (match != true) {
        throw new BadSecretException("Secret failed local check.");
      }

      byte[] userNonce = mChallenge.extractNonce(userGamma);
      byte[] genAlpha = mChallenge.getAlpha(userNonce);

      logDebug("Sending Secret reply...");
      SecretMessage secretMessage = new SecretMessage(genAlpha);
      mProtocol.sendMessage(secretMessage);

      logDebug("Waiting for SecretAck...");
      SecretAckMessage secretAck =
          (SecretAckMessage) getNextMessage(PoloMessageType.SECRET_ACK);

      if (VERIFY_SECRET_ACK) {
        byte[] inbandAlpha = secretAck.getSecret();
        if (!Arrays.equals(inbandAlpha, genAlpha)) {
          throw new BadSecretException("Inband secret did not match. " +
              "Expected [" + PoloUtil.bytesToHexString(genAlpha) +
              "], got [" + PoloUtil.bytesToHexString(inbandAlpha) + "]");
        }
      }
    } else {
      int symbolLength = mSessionConfig.getEncoding().getSymbolLength();
      int nonceLength = symbolLength / 2;
      int bytesNeeded = nonceLength / mEncoder.symbolsPerByte();

      byte[] nonce = new byte[bytesNeeded];
      SecureRandom random;
      try {
        random = SecureRandom.getInstance("SHA1PRNG");
      } catch (NoSuchAlgorithmException e) {
        throw new PoloException(e);
      }
      random.nextBytes(nonce);

      // Display gamma
      logDebug("Calling listener to display output...");
      byte[] gamma = mChallenge.getGamma(nonce);
      mListener.onPerformOutputDeviceRole(this, gamma);

      logDebug("Waiting for Secret...");
      SecretMessage secretMessage =
          (SecretMessage) getNextMessage(PoloMessageType.SECRET);

      byte[] localAlpha = mChallenge.getAlpha(nonce);
      byte[] inbandAlpha = secretMessage.getSecret();
      boolean matched = Arrays.equals(localAlpha, inbandAlpha);

      if (!matched) {
        throw new BadSecretException("Inband secret did not match. " +
            "Expected [" + PoloUtil.bytesToHexString(localAlpha) +
            "], got [" + PoloUtil.bytesToHexString(inbandAlpha) + "]");
      }

      logDebug("Sending SecretAck...");
      byte[] genAlpha = mChallenge.getAlpha(nonce);
      SecretAckMessage secretAck = new SecretAckMessage(inbandAlpha);
      mProtocol.sendMessage(secretAck);
    }
  }

  public SecretEncoder getEncoder() {
    return mEncoder;
  }

  /**
   * Sets the current session's configuration from a
   * {@link ConfigurationMessage}.
   *
   * @param message         the session's config
   * @throws PoloException  if the config was not valid for some reason
   */
  protected void setConfiguration(ConfigurationMessage message)
      throws PoloException {
    if (message == null || message.getEncoding() == null) {
      throw new NoConfigurationException("No configuration is possible.");
    }
    if (message.getEncoding().getSymbolLength() % 2 != 0) {
      throw new PoloException("Symbol length must be even.");
    }
    if (message.getEncoding().getSymbolLength() < 2) {
      throw new PoloException("Symbol length must be >= 2 symbols.");
    }
    switch (message.getEncoding().getType()) {
      case ENCODING_HEXADECIMAL:
        mEncoder = new HexadecimalEncoder();
        break;
      default:
        throw new PoloException("Unsupported encoding type.");
    }
    mSessionConfig = message;
  }

  /**
   * Returns the role of this endpoint in the current session.
   */
  protected ProtocolRole getLocalRole() {
    assert (mSessionConfig != null);
    if (!mPairingContext.isServer()) {
      return mSessionConfig.getClientRole();
    } else {
      return (mSessionConfig.getClientRole() == ProtocolRole.DISPLAY_DEVICE) ?
          ProtocolRole.INPUT_DEVICE : ProtocolRole.DISPLAY_DEVICE;
    }
  }

  /**
   * Returns {@code true} if this endpoint will act as the input device.
   */
  protected boolean isInputDevice() {
    return (getLocalRole() == ProtocolRole.INPUT_DEVICE);
  }

  /**
   * Returns {@code true} if peer's name is set.
   */
  public boolean hasPeerName() {
    return mPeerName != null;
  }

  /**
   * Returns peer's name if set, {@code null} otherwise.
   */
  public String getPeerName() {
    return mPeerName;
  }

  protected PoloMessage getNextMessage(PoloMessageType type)
      throws PoloException {
    QueueMessage message = waitForMessage();
    if (message != null && message.hasPoloMessage()) {
      if (!type.equals(message.mPoloMessage.getType())) {
        throw new PoloException(
            "Unexpected message type: " + message.mPoloMessage.getType());
      }
      return message.mPoloMessage;
    }
    throw new PoloException("Invalid state - expected polo message");
  }

  /**
   * Returns next queued message. The method blocks until the secret or the
   * polo message is available.
   *
   * @return the queued message, or null on error
   * @throws PoloException if exception was queued
   */
  private QueueMessage waitForMessage() throws PoloException {
    while (!mAbort) {
      try {
        QueueMessage message = mMessageQueue.poll(SECRET_POLL_TIMEOUT_MS,
            TimeUnit.MILLISECONDS);

        if (message != null) {
          if (message.hasPoloException()) {
            throw new PoloException(message.mPoloException);
          }
          return message;
        }
      } catch (InterruptedException e) {
        break;
      }
    }

    // Aborted or interrupted.
    return null;
  }

  /**
   * Sends message to the peer.
   *
   * @param message         the message
   * @throws PoloException  if a protocol error occurred
   * @throws IOException    if an error in the input/output occurred
   */
  protected void sendMessage(PoloMessage message)
      throws IOException, PoloException {
    mProtocol.sendMessage(message);
  }

  /**
   * Queued message, that can carry information about secret, next read message,
   * or exception caught by reader or input threads.
   */
  private static final class QueueMessage {
    final PoloMessage mPoloMessage;
    final PoloException mPoloException;
    final byte[] mSecret;

    private QueueMessage(
        PoloMessage message, byte[] secret, PoloException exception) {
      int nonNullCount = 0;
      if (message != null) {
        ++nonNullCount;
      }
      mPoloMessage = message;
      if (exception != null) {
        assert(nonNullCount == 0);
        ++nonNullCount;
      }
      mPoloException = exception;
      if (secret != null) {
        assert(nonNullCount == 0);
        ++nonNullCount;
      }
      mSecret = secret;
      assert(nonNullCount == 1);
    }

    public QueueMessage(PoloMessage message) {
      this(message, null, null);
    }

    public QueueMessage(byte[] secret) {
      this(null, secret, null);
    }

    public QueueMessage(PoloException exception) {
      this(null, null, exception);
    }

    public boolean hasPoloMessage() {
      return mPoloMessage != null;
    }

    public boolean hasPoloException() {
      return mPoloException != null;
    }

    public boolean hasSecret() {
      return mSecret != null;
    }

    @Override
    public String toString() {
      StringBuilder builder = new StringBuilder("QueueMessage(");
      if (hasPoloMessage()) {
        builder.append("poloMessage = " + mPoloMessage);
      }
      if (hasPoloException()) {
        builder.append("poloException = " + mPoloException);
      }
      if (hasSecret()) {
        builder.append("secret = " + Arrays.toString(mSecret));
      }
      return builder.append(")").toString();
    }
  }

}
