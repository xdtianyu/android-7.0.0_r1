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
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;

/**
 * Test (fake) subclass of ServerSocket that performs no network access and allows setting the 
 * Socket returned by accept(), and the local port for the ServerSocket.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public class StubServerSocket extends ServerSocket {
    private int localPort;
    private Socket socket;

    /**
     * Construct a new instance with the specified local port.
     * @param localPort - the local port to be returned from getLocalPort()
     * @throws IOException
     */
    public StubServerSocket(int localPort) throws IOException {
        this(localPort, null);
    }

    /**
     * Construct a new instance with specified local port and accept() socket. 
     * @param localPort - the local port to be returned from getLocalPort()
     * @param socket - the socket to be returned from accept(); if null, then accept() throws SocketTimeoutException. 
     * @throws IOException
     */
    public StubServerSocket(int localPort, Socket socket) throws IOException {
        super(0);
        this.localPort = localPort;
        this.socket = socket;
    }
    
    /**
     * Return the predefined local port 
     * @see java.net.ServerSocket#getLocalPort()
     */
    public int getLocalPort() {
        return localPort;
    }
    
    /**
     * If a socket was specified on the constructor, then return that; otherwise, throw a SocketTimeoutException. 
     * @see java.net.ServerSocket#accept()
     */
    public Socket accept() throws IOException {
        if (socket != null) {
            return socket;
        }
        throw new SocketTimeoutException();
    }
}
