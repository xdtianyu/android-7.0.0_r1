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

package com.squareup.okhttp;

import java.net.Proxy;
import java.util.Arrays;
import java.util.List;

import javax.net.ssl.HttpsURLConnection;

public final class HttpsHandler extends HttpHandler {

    /**
     * The initial connection spec to use when connecting to an https:// server, and the prototype
     * for the others below. Note that Android does not set the cipher suites to use so the socket's
     * defaults enabled cipher suites will be used instead. When the SSLSocketFactory is provided by
     * the app or GMS core we will not override the enabled ciphers set on the sockets it produces
     * with a list hardcoded at release time. This is deliberate.
     * For the TLS versions we <em>will</em> select a known subset from the set of enabled TLS
     * versions on the socket.
     */
    private static final ConnectionSpec TLS_1_2_AND_BELOW = new ConnectionSpec.Builder(true)
        .tlsVersions(TlsVersion.TLS_1_2, TlsVersion.TLS_1_1, TlsVersion.TLS_1_0, TlsVersion.SSL_3_0)
        .supportsTlsExtensions(true)
        .build();

    private static final ConnectionSpec TLS_1_1_AND_BELOW =
        new ConnectionSpec.Builder(TLS_1_2_AND_BELOW)
            .tlsVersions(TlsVersion.TLS_1_1, TlsVersion.TLS_1_0, TlsVersion.SSL_3_0)
            .supportsTlsExtensions(true)
            .build();

    private static final ConnectionSpec TLS_1_0_AND_BELOW =
        new ConnectionSpec.Builder(TLS_1_2_AND_BELOW)
            .tlsVersions(TlsVersion.TLS_1_0, TlsVersion.SSL_3_0)
            .build();

    private static final ConnectionSpec SSL_3_0 =
        new ConnectionSpec.Builder(TLS_1_2_AND_BELOW)
            .tlsVersions(TlsVersion.SSL_3_0)
            .build();

    /** Try up to 4 times to negotiate a connection with each server. */
    private static final List<ConnectionSpec> SECURE_CONNECTION_SPECS =
        Arrays.asList(TLS_1_2_AND_BELOW, TLS_1_1_AND_BELOW, TLS_1_0_AND_BELOW, SSL_3_0);

    private static final List<Protocol> HTTP_1_1_ONLY = Arrays.asList(Protocol.HTTP_1_1);

    private final ConfigAwareConnectionPool configAwareConnectionPool =
            ConfigAwareConnectionPool.getInstance();

    @Override protected int getDefaultPort() {
        return 443;
    }

    @Override
    protected OkUrlFactory newOkUrlFactory(Proxy proxy) {
        OkUrlFactory okUrlFactory = createHttpsOkUrlFactory(proxy);
        // For HttpsURLConnections created through java.net.URL Android uses a connection pool that
        // is aware when the default network changes so that pooled connections are not re-used when
        // the default network changes.
        okUrlFactory.client().setConnectionPool(configAwareConnectionPool.get());
        return okUrlFactory;
    }

    /**
     * Creates an OkHttpClient suitable for creating {@link HttpsURLConnection} instances on
     * Android.
     */
    // Visible for android.net.Network.
    public static OkUrlFactory createHttpsOkUrlFactory(Proxy proxy) {
        // The HTTPS OkHttpClient is an HTTP OkHttpClient with extra configuration.
        OkUrlFactory okUrlFactory = HttpHandler.createHttpOkUrlFactory(proxy);

        // All HTTPS requests are allowed.
        okUrlFactory.setUrlFilter(null);

        OkHttpClient okHttpClient = okUrlFactory.client();

        // Only enable HTTP/1.1 (implies HTTP/1.0). Disable SPDY / HTTP/2.0.
        okHttpClient.setProtocols(HTTP_1_1_ONLY);

        // Use Android's preferred fallback approach and cipher suite selection.
        okHttpClient.setConnectionSpecs(SECURE_CONNECTION_SPECS);

        // OkHttp does not automatically honor the system-wide HostnameVerifier set with
        // HttpsURLConnection.setDefaultHostnameVerifier().
        okUrlFactory.client().setHostnameVerifier(HttpsURLConnection.getDefaultHostnameVerifier());
        // OkHttp does not automatically honor the system-wide SSLSocketFactory set with
        // HttpsURLConnection.setDefaultSSLSocketFactory().
        // See https://github.com/square/okhttp/issues/184 for details.
        okHttpClient.setSslSocketFactory(HttpsURLConnection.getDefaultSSLSocketFactory());

        return okUrlFactory;
    }
}
