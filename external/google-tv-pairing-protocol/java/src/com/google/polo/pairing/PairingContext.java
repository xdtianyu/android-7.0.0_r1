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

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.cert.Certificate;

import javax.net.ssl.SSLSocket;

/**
 * Container class for various bits of state related to a pairing session.
 */
public class PairingContext {

  /**
   * The {@link Certificate} of the local endpoint of the protocol.
   */
  private Certificate mLocalCertificate;
  
  /**
   * The {@link Certificate} of the remote endpoint of the protocol.
   */
  private Certificate mPeerCertificate;
  
  /**
   * An {@link InputStream} for the peer in the protocol.
   */
  private InputStream mPeerInputStream;
  
  /**
   * An {@link OutputStream} for the peer in the protocol.
   */
  private OutputStream mPeerOutputStream;
  
  /**
   * {@code true} if this context is for a server endpoint.
   */
  private final boolean mIsServer;

  /**
   * Constructs a new instance.
   * 
   * @param localCertificate  the local endpoint's {@link Certificate}
   * @param peerCertificate   the remote endpoint's {@link Certificate}
   * @param peerInputStream   an {@link InputStream} from the peer
   * @param peerOutputStream  a {@link OutputStream} to the peer
   * @param isServer          {@code true} if this endpoint it the server
   */
  public PairingContext(Certificate localCertificate,
      Certificate peerCertificate, InputStream peerInputStream,
      OutputStream peerOutputStream, boolean isServer) {
    setLocalCertificate(localCertificate);
    setPeerCertificate(peerCertificate);
    setPeerInputStream(peerInputStream);
    setPeerOutputStream(peerOutputStream);
    mIsServer = isServer;
  }
  
  /**
   * Constructs a new instance from an {@link SSLSocket}.
   * 
   * @param   socket          the socket to use
   * @param   isServer        {@code true} if this endpoint is the server
   * @return  the new instance
   * @throws PoloException  if certificates could not be obtained
   * @throws IOException    if the socket's streams could not be obtained
   */
  public static PairingContext fromSslSocket(SSLSocket socket, boolean isServer)
      throws PoloException, IOException {
    Certificate localCert = PoloUtil.getLocalCert(socket.getSession());
    Certificate peerCert = PoloUtil.getPeerCert(socket.getSession());
    InputStream input = socket.getInputStream();
    OutputStream output = socket.getOutputStream();
    return new PairingContext(localCert, peerCert, input, output, isServer);
  }

  public void setLocalCertificate(Certificate localCertificate) {
    mLocalCertificate = localCertificate;
  }

  public Certificate getClientCertificate() {
    if (isServer()) {
      return mPeerCertificate;
    } else {
      return mLocalCertificate;
    }
  }

  public void setPeerCertificate(Certificate peerCertificate) {
    mPeerCertificate = peerCertificate;
  }

  public Certificate getServerCertificate() {
    if (isServer()) {
      return mLocalCertificate;
    } else {
      return mPeerCertificate;
    }
  }

  public void setPeerInputStream(InputStream peerInputStream) {
    mPeerInputStream = peerInputStream;
  }

  public InputStream getPeerInputStream() {
    return mPeerInputStream;
  }

  public void setPeerOutputStream(OutputStream peerOutputStream) {
    mPeerOutputStream = peerOutputStream;
  }

  public OutputStream getPeerOutputStream() {
    return mPeerOutputStream;
  }
  
  public boolean isServer() {
    return mIsServer;
  }
  
  public boolean isClient() {
    return !(isServer());
  }
  
}
