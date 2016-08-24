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

/**
 * Default implementation of the {@link SocketFactory}; creates standard {@link Socket} instances.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public class DefaultSocketFactory implements SocketFactory {
    
    /**
     * Create a new Socket instance for the specified host and port.
     * @param host - the IP address of the host endpoint to which the socket is connect
     * @param port - the port number of the enpoint to which the socket is connected
     * @return a new Socket
     * @throws IOException
     * 
     * @see org.mockftpserver.core.socket.SocketFactory#createSocket(java.net.InetAddress, int)
     */
    public Socket createSocket(InetAddress host, int port) throws IOException {
        return new Socket(host, port);
    }
}