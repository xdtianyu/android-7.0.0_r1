/*
 * Copyright (C) 2012 Square, Inc.
 * Copyright (C) 2012 The Android Open Source Project
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
package com.squareup.okhttp.internal;

import dalvik.system.SocketTagger;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.List;
import javax.net.ssl.SSLSocket;

import com.squareup.okhttp.Protocol;

import okio.Buffer;

/**
 * Access to proprietary Android APIs. Doesn't use reflection.
 */
public final class Platform {
    private static final Platform PLATFORM = new Platform();

    public static Platform get() {
        return PLATFORM;
    }

    /** setUseSessionTickets(boolean) */
    private static final OptionalMethod<Socket> SET_USE_SESSION_TICKETS =
            new OptionalMethod<Socket>(null, "setUseSessionTickets", Boolean.TYPE);
    /** setHostname(String) */
    private static final OptionalMethod<Socket> SET_HOSTNAME =
            new OptionalMethod<Socket>(null, "setHostname", String.class);
    /** byte[] getAlpnSelectedProtocol() */
    private static final OptionalMethod<Socket> GET_ALPN_SELECTED_PROTOCOL =
            new OptionalMethod<Socket>(byte[].class, "getAlpnSelectedProtocol");
    /** setAlpnSelectedProtocol(byte[]) */
    private static final OptionalMethod<Socket> SET_ALPN_PROTOCOLS =
            new OptionalMethod<Socket>(null, "setAlpnProtocols", byte[].class );

    public void logW(String warning) {
        System.logW(warning);
    }

    public void tagSocket(Socket socket) throws SocketException {
        SocketTagger.get().tag(socket);
    }

    public void untagSocket(Socket socket) throws SocketException {
        SocketTagger.get().untag(socket);
    }

    public void configureTlsExtensions(
            SSLSocket sslSocket, String hostname, List<Protocol> protocols) {
        // Enable SNI and session tickets.
        if (hostname != null) {
            SET_USE_SESSION_TICKETS.invokeOptionalWithoutCheckedException(sslSocket, true);
            SET_HOSTNAME.invokeOptionalWithoutCheckedException(sslSocket, hostname);
        }

        // Enable ALPN.
        boolean alpnSupported = SET_ALPN_PROTOCOLS.isSupported(sslSocket);
        if (!alpnSupported) {
            return;
        }

        Object[] parameters = { concatLengthPrefixed(protocols) };
        if (alpnSupported) {
            SET_ALPN_PROTOCOLS.invokeWithoutCheckedException(sslSocket, parameters);
        }
    }

    /**
     * Called after the TLS handshake to release resources allocated by {@link
     * #configureTlsExtensions}.
     */
    public void afterHandshake(SSLSocket sslSocket) {
    }

    public String getSelectedProtocol(SSLSocket socket) {
        boolean alpnSupported = GET_ALPN_SELECTED_PROTOCOL.isSupported(socket);
        if (!alpnSupported) {
            return null;
        }

        byte[] alpnResult =
                (byte[]) GET_ALPN_SELECTED_PROTOCOL.invokeWithoutCheckedException(socket);
        if (alpnResult != null) {
            return new String(alpnResult, Util.UTF_8);
        }
        return null;
    }

    public void connectSocket(Socket socket, InetSocketAddress address,
              int connectTimeout) throws IOException {
        socket.connect(address, connectTimeout);
    }

    /** Prefix used on custom headers. */
    public String getPrefix() {
        return "X-Android";
    }

    /**
     * Returns the concatenation of 8-bit, length prefixed protocol names.
     * http://tools.ietf.org/html/draft-agl-tls-nextprotoneg-04#page-4
     */
    static byte[] concatLengthPrefixed(List<Protocol> protocols) {
        Buffer result = new Buffer();
        for (int i = 0, size = protocols.size(); i < size; i++) {
            Protocol protocol = protocols.get(i);
            if (protocol == Protocol.HTTP_1_0) continue; // No HTTP/1.0 for ALPN.
            result.writeByte(protocol.toString().length());
            result.writeUtf8(protocol.toString());
        }
        return result.readByteArray();
    }
}
