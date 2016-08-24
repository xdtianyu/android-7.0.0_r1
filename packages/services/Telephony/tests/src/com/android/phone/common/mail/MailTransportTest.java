/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.phone.common.mail;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.net.Network;
import android.test.AndroidTestCase;

import com.android.phone.MockitoHelper;
import com.android.phone.common.mail.MailTransport.SocketCreator;
import com.android.phone.common.mail.store.ImapStore;
import com.android.phone.vvm.omtp.imap.ImapHelper;

import junit.framework.AssertionFailedError;

import org.mockito.MockitoAnnotations;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.UnknownHostException;

import javax.net.SocketFactory;

public class MailTransportTest extends AndroidTestCase {

    private static final String HOST_ADDRESS = "127.0.0.1";
    private static final String INVALID_HOST_ADDRESS = "255.255.255.255";
    private static final int HOST_PORT = 80;
    private static final int HOST_FLAGS = 0;
    // bypass verifyHostname() in open() by setting ImapStore.FLAG_TRUST_ALL
    private static final int HOST_FLAGS_SSL = ImapStore.FLAG_SSL & ImapStore.FLAG_TRUST_ALL;
    private static final InetAddress VALID_INET_ADDRESS = createInetAddress(HOST_ADDRESS);
    private static final InetAddress INVALID_INET_ADDRESS = createInetAddress(INVALID_HOST_ADDRESS);

    // ClassLoader need to be replaced for mockito to work.
    private MockitoHelper mMokitoHelper = new MockitoHelper();

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mMokitoHelper.setUp(getClass());
        MockitoAnnotations.initMocks(this);
    }

    @Override
    public void tearDown() throws Exception {
        mMokitoHelper.tearDown();
        super.tearDown();
    }

    public void testCreateSocket_anyNetwork() throws MessagingException {
        // With no network, Socket#Socket() should be called.
        MailTransport transport =
                new MailTransport(getContext(), createMockImapHelper(), null, HOST_ADDRESS,
                        HOST_PORT, HOST_FLAGS);
        Socket socket = transport.createSocket();
        assertTrue(socket != null);
    }

    public void testCreateSocket_networkSpecified() throws MessagingException, IOException {
        // Network#getSocketFactory should be used to create socket.
        Network mockNetwork = createMockNetwork();
        MailTransport transport =
                new MailTransport(getContext(), createMockImapHelper(), mockNetwork, HOST_ADDRESS,
                        HOST_PORT, HOST_FLAGS);
        Socket socket = transport.createSocket();
        assertTrue(socket != null);
        verify(mockNetwork).getSocketFactory();
    }

    public void testCreateSocket_socketCreator() throws MessagingException, IOException {
        // For testing purposes, how sockets are created can be overridden.
        SocketCreator socketCreator = new SocketCreator() {

            private final Socket mSocket = new Socket();

            @Override
            public Socket createSocket() {
                return mSocket;
            }
        };

        MailTransport transport = new
                MailTransport(getContext(), createMockImapHelper(), null, HOST_ADDRESS, HOST_PORT,
                HOST_FLAGS);

        transport.setSocketCreator(socketCreator);

        Socket socket = transport.createSocket();
        assertTrue(socket == socketCreator.createSocket());
    }

    public void testOpen() throws MessagingException {
        MailTransport transport = new MailTransport(getContext(), createMockImapHelper(), null,
                HOST_ADDRESS,
                HOST_PORT, HOST_FLAGS);
        transport.setSocketCreator(new TestSocketCreator());
        transport.open();
        assertTrue(transport.isOpen());

    }

    public void testOpen_Ssl() throws MessagingException {
        //opening with ssl support.
        MailTransport transport = new MailTransport(getContext(), createMockImapHelper(), null,
                HOST_ADDRESS, HOST_PORT, HOST_FLAGS_SSL);
        transport.setSocketCreator(new TestSocketCreator());
        transport.open();
        assertTrue(transport.isOpen());

    }

    public void testOpen_MultiIp() throws MessagingException {
        //In case of round robin DNS, try all resolved address until one succeeded.
        Network network = createMultiIpMockNetwork();
        MailTransport transport = new MailTransport(getContext(), createMockImapHelper(), network,
                HOST_ADDRESS,
                HOST_PORT, HOST_FLAGS);
        transport.setSocketCreator(new TestSocketCreator());
        transport.open();
        assertTrue(transport.isOpen());
    }

    public void testOpen_MultiIp_SSL() throws MessagingException {
        Network network = createMultiIpMockNetwork();

        MailTransport transport = new MailTransport(getContext(), createMockImapHelper(), network,
                HOST_ADDRESS,
                HOST_PORT, HOST_FLAGS_SSL);
        transport.setSocketCreator(new TestSocketCreator());
        transport.open();
        assertTrue(transport.isOpen());
    }

    public void testOpen_network_hostResolutionFailed() {
        // Couldn't resolve host on the network. Open() should fail.
        Network network = createMockNetwork();
        try {
            when(network.getAllByName(HOST_ADDRESS))
                    .thenThrow(new UnknownHostException("host resolution failed"));
        } catch (IOException e) {
            //ignored
        }

        MailTransport transport = new MailTransport(getContext(), createMockImapHelper(), network,
                HOST_ADDRESS,
                HOST_PORT, HOST_FLAGS);
        try {
            transport.open();
            throw new AssertionFailedError("Should throw MessagingException");
        } catch (MessagingException e) {
            //expected
        }
        assertFalse(transport.isOpen());
    }

    public void testOpen_createSocketFailed() {
        // Unable to create socket. Open() should fail.
        MailTransport transport = new MailTransport(getContext(), createMockImapHelper(), null,
                HOST_ADDRESS,
                HOST_PORT, HOST_FLAGS);
        transport.setSocketCreator(new SocketCreator() {
            @Override
            public Socket createSocket() throws MessagingException {
                throw new MessagingException("createSocket failed");
            }
        });
        try {
            transport.open();
            throw new AssertionFailedError("Should throw MessagingException");
        } catch (MessagingException e) {
            //expected
        }
        assertFalse(transport.isOpen());
    }

    public void testOpen_network_createSocketFailed() {
        // Unable to create socket. Open() should fail.

        Network network = createOneIpMockNetwork();
        SocketFactory mockSocketFactory = mock(SocketFactory.class);
        try {
            when(mockSocketFactory.createSocket())
                    .thenThrow(new IOException("unable to create socket"));
        } catch (IOException e) {
            //ignored
        }
        when(network.getSocketFactory()).thenReturn(mockSocketFactory);

        MailTransport transport = new MailTransport(getContext(), createMockImapHelper(), network,
                HOST_ADDRESS, HOST_PORT, HOST_FLAGS);

        try {
            transport.open();
            throw new AssertionFailedError("Should throw MessagingException");
        } catch (MessagingException e) {
            //expected
        }
        assertFalse(transport.isOpen());
    }

    public void testOpen_connectFailed_one() {
        // There is only one IP for this host, and we failed to connect to it. Open() should fail.

        MailTransport transport = new MailTransport(getContext(), createMockImapHelper(), null,
                HOST_ADDRESS, HOST_PORT, HOST_FLAGS);
        transport.setSocketCreator(new SocketCreator() {
            @Override
            public Socket createSocket() throws MessagingException {
                return new Socket() {
                    @Override
                    public void connect(SocketAddress address, int timeout) throws IOException {
                        throw new IOException("connect failed");
                    }
                };
            }
        });
        try {
            transport.open();
            throw new AssertionFailedError("Should throw MessagingException");
        } catch (MessagingException e) {
            //expected
        }
        assertFalse(transport.isOpen());
    }

    public void testOpen_connectFailed_multi() {
        // There are multiple IP for this host, and we failed to connect to any of it.
        // Open() should fail.
        MailTransport transport = new MailTransport(getContext(), createMockImapHelper(),
                createMultiIpMockNetwork(), HOST_ADDRESS, HOST_PORT, HOST_FLAGS);
        transport.setSocketCreator(new SocketCreator() {
            @Override
            public Socket createSocket() throws MessagingException {
                return new Socket() {
                    @Override
                    public void connect(SocketAddress address, int timeout) throws IOException {
                        throw new IOException("connect failed");
                    }
                };
            }
        });
        try {
            transport.open();
            throw new AssertionFailedError("Should throw MessagingException");
        } catch (MessagingException e) {
            //expected
        }
        assertFalse(transport.isOpen());
    }

    private class TestSocket extends Socket {

        boolean mConnected = false;


        /**
         * A make a mock connection to the address.
         *
         * @param address Only address equivalent to VALID_INET_ADDRESS or INVALID_INET_ADDRESS is
         * accepted
         * @param timeout Ignored but should >= 0.
         */
        @Override
        public void connect(SocketAddress address, int timeout) throws IOException {
            // copied from Socket#connect
            if (isClosed()) {
                throw new SocketException("Socket is closed");
            }
            if (timeout < 0) {
                throw new IllegalArgumentException("timeout < 0");
            }
            if (isConnected()) {
                throw new SocketException("Already connected");
            }
            if (address == null) {
                throw new IllegalArgumentException("remoteAddr == null");
            }

            if (!(address instanceof InetSocketAddress)) {
                throw new AssertionError("address should be InetSocketAddress");
            }

            InetSocketAddress inetSocketAddress = (InetSocketAddress) address;
            if (inetSocketAddress.getAddress().equals(INVALID_INET_ADDRESS)) {
                throw new IOException("invalid address");
            } else if (inetSocketAddress.getAddress().equals(VALID_INET_ADDRESS)) {
                mConnected = true;
            } else {
                throw new AssertionError("Only INVALID_ADDRESS or VALID_ADDRESS are allowed");
            }
        }

        @Override
        public InputStream getInputStream() {
            return null;
        }

        @Override
        public OutputStream getOutputStream() {
            return null;
        }

        @Override
        public boolean isConnected() {
            return mConnected;
        }

    }


    private class TestSocketCreator implements MailTransport.SocketCreator {

        @Override
        public Socket createSocket() throws MessagingException {
            Socket socket = new TestSocket();
            return socket;
        }

    }

    private ImapHelper createMockImapHelper() {
        return mock(ImapHelper.class);
    }

    /**
     * @return a mock Network that can create a TestSocket with {@code getSocketFactory()
     * .createSocket()}
     */
    private Network createMockNetwork() {
        Network network = mock(Network.class);
        SocketFactory mockSocketFactory = mock(SocketFactory.class);
        try {
            when(mockSocketFactory.createSocket()).thenReturn(new TestSocket());
        } catch (IOException e) {
            //ignored
        }
        when(network.getSocketFactory()).thenReturn(mockSocketFactory);
        return network;
    }

    /**
     * @return a mock Network like {@link MailTransportTest#createMockNetwork()}, but also supports
     * {@link Network#getAllByName(String)} with one valid result.
     */
    private Network createOneIpMockNetwork() {
        Network network = createMockNetwork();
        try {
            when(network.getAllByName(HOST_ADDRESS))
                    .thenReturn(new InetAddress[] {VALID_INET_ADDRESS});
        } catch (UnknownHostException e) {
            //ignored
        }

        return network;
    }

    /**
     * @return a mock Network like {@link MailTransportTest#createMockNetwork()}, but also supports
     * {@link Network#getAllByName(String)}, which will return 2 address with the first one
     * invalid.
     */
    private Network createMultiIpMockNetwork() {
        Network network = createMockNetwork();
        try {
            when(network.getAllByName(HOST_ADDRESS))
                    .thenReturn(new InetAddress[] {INVALID_INET_ADDRESS, VALID_INET_ADDRESS});
        } catch (UnknownHostException e) {
            //ignored
        }

        return network;
    }

    /**
     * helper method to translate{@code host} into a InetAddress.
     *
     * @param host IP address of the host. Domain name should not be used as this method should not
     * access the internet.
     */
    private static InetAddress createInetAddress(String host) {
        try {
            return InetAddress.getByName(host);
        } catch (UnknownHostException e) {
            return null;
        }
    }


}
