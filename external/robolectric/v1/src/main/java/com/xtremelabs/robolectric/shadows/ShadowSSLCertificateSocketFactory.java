package com.xtremelabs.robolectric.shadows;

import com.xtremelabs.robolectric.internal.Implementation;
import com.xtremelabs.robolectric.internal.Implements;

import org.apache.http.conn.ssl.SSLSocketFactory;

import android.net.SSLCertificateSocketFactory;
import android.net.SSLSessionCache;

@Implements(SSLCertificateSocketFactory.class)
public class ShadowSSLCertificateSocketFactory {

    // TODO: Support more features when necessary
    @Implementation
    public static SSLSocketFactory getHttpSocketFactory(
            int handshakeTimeoutMillis,
            SSLSessionCache cache) {
        return SSLSocketFactory.getSocketFactory();
    }
}