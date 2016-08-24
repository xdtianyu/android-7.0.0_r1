/*
 * Copyright 2007 the original author or authors.
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
package org.mockftpserver.core.socket;

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;

import org.mockftpserver.core.socket.SocketFactory;

/**
 * Test-only implementation of SocketFactory. It always returns the predefined
 * StubSocket instance specified on the constructor. It allows direct access to the
 * requested host address and port number.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public class StubSocketFactory implements SocketFactory {
    private StubSocket stubSocket;
    public int requestedDataPort;
    public InetAddress requestedHost;

    /**
     * Create a new instance that always returns the specified StubSocket instance.
     * @param stubSocket - the StubSocket to be returned by this factory
     */
    public StubSocketFactory(StubSocket stubSocket) {
        this.stubSocket = stubSocket;
    }

    /**
     * Return the predefined StubSocket instance
     * @see org.mockftpserver.core.socket.SocketFactory#createSocket(java.net.InetAddress, int)
     */
    public Socket createSocket(InetAddress host, int port) throws IOException {
        this.requestedHost = host;
        this.requestedDataPort = port;
        return stubSocket;
    }
}