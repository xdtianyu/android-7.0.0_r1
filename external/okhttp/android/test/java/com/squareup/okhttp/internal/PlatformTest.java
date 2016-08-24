/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package com.squareup.okhttp.internal;

import com.android.org.conscrypt.OpenSSLSocketImpl;
import com.squareup.okhttp.Protocol;

import org.junit.Test;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.List;

import javax.net.ssl.HandshakeCompletedListener;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;

import okio.ByteString;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

/**
 * Tests for {@link Platform}.
 */
public class PlatformTest {

  @Test
  public void enableTlsExtensionOptionalMethods() throws Exception {
    Platform platform = new Platform();

    // Expect no error
    TestSSLSocketImpl arbitrarySocketImpl = new TestSSLSocketImpl();
    List<Protocol> protocols = Arrays.asList(Protocol.HTTP_1_1, Protocol.SPDY_3);
    platform.configureTlsExtensions(arbitrarySocketImpl, "host", protocols);
    NpnOnlySSLSocketImpl npnOnlySSLSocketImpl = new NpnOnlySSLSocketImpl();
    platform.configureTlsExtensions(npnOnlySSLSocketImpl, "host", protocols);

    FullOpenSSLSocketImpl openSslSocket = new FullOpenSSLSocketImpl();
    platform.configureTlsExtensions(openSslSocket, "host", protocols);
    assertTrue(openSslSocket.useSessionTickets);
    assertEquals("host", openSslSocket.hostname);
    assertArrayEquals(Platform.concatLengthPrefixed(protocols), openSslSocket.alpnProtocols);
  }

  @Test
  public void getSelectedProtocol() throws Exception {
    Platform platform = new Platform();
    String selectedProtocol = "alpn";

    TestSSLSocketImpl arbitrarySocketImpl = new TestSSLSocketImpl();
    assertNull(platform.getSelectedProtocol(arbitrarySocketImpl));

    NpnOnlySSLSocketImpl npnOnlySSLSocketImpl = new NpnOnlySSLSocketImpl();
    assertNull(platform.getSelectedProtocol(npnOnlySSLSocketImpl));

    FullOpenSSLSocketImpl openSslSocket = new FullOpenSSLSocketImpl();
    openSslSocket.alpnProtocols = selectedProtocol.getBytes(StandardCharsets.UTF_8);
    assertEquals(selectedProtocol, platform.getSelectedProtocol(openSslSocket));
  }

  private static class FullOpenSSLSocketImpl extends OpenSSLSocketImpl {
    private boolean useSessionTickets;
    private String hostname;
    private byte[] alpnProtocols;

    public FullOpenSSLSocketImpl() throws IOException {
      super(null);
    }

    @Override
    public void setUseSessionTickets(boolean useSessionTickets) {
      this.useSessionTickets = useSessionTickets;
    }

    @Override
    public void setHostname(String hostname) {
      this.hostname = hostname;
    }

    @Override
    public void setAlpnProtocols(byte[] alpnProtocols) {
      this.alpnProtocols = alpnProtocols;
    }

    @Override
    public byte[] getAlpnSelectedProtocol() {
      return alpnProtocols;
    }
  }

  // Legacy case - NPN support has been dropped.
  private static class NpnOnlySSLSocketImpl extends TestSSLSocketImpl {

    private byte[] npnProtocols;

    public void setNpnProtocols(byte[] npnProtocols) {
      this.npnProtocols = npnProtocols;
    }

    public byte[] getNpnSelectedProtocol() {
      return npnProtocols;
    }
  }

  private static class TestSSLSocketImpl extends SSLSocket {

    @Override
    public String[] getSupportedCipherSuites() {
      return new String[0];
    }

    @Override
    public String[] getEnabledCipherSuites() {
      return new String[0];
    }

    @Override
    public void setEnabledCipherSuites(String[] suites) {
    }

    @Override
    public String[] getSupportedProtocols() {
      return new String[0];
    }

    @Override
    public String[] getEnabledProtocols() {
      return new String[0];
    }

    @Override
    public void setEnabledProtocols(String[] protocols) {
    }

    @Override
    public SSLSession getSession() {
      return null;
    }

    @Override
    public void addHandshakeCompletedListener(HandshakeCompletedListener listener) {
    }

    @Override
    public void removeHandshakeCompletedListener(HandshakeCompletedListener listener) {
    }

    @Override
    public void startHandshake() throws IOException {
    }

    @Override
    public void setUseClientMode(boolean mode) {
    }

    @Override
    public boolean getUseClientMode() {
      return false;
    }

    @Override
    public void setNeedClientAuth(boolean need) {
    }

    @Override
    public void setWantClientAuth(boolean want) {
    }

    @Override
    public boolean getNeedClientAuth() {
      return false;
    }

    @Override
    public boolean getWantClientAuth() {
      return false;
    }

    @Override
    public void setEnableSessionCreation(boolean flag) {
    }

    @Override
    public boolean getEnableSessionCreation() {
      return false;
    }
  }
}
