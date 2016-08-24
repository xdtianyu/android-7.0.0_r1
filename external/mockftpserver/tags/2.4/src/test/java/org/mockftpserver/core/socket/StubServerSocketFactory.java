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

/**
 * Test-only implementation of ServerSocketFactory. It always returns the predefined
 * ServerSocket instance specified on the constructor.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public class StubServerSocketFactory implements ServerSocketFactory {
    private StubServerSocket stubServerSocket;

    /**
     * Construct a new factory instance that always returns the specified
     * ServerSocket instance. 
     * @param serverSocket - the ServerSocket instance to be returned by this factory
     */
    public StubServerSocketFactory(StubServerSocket serverSocket) {
        this.stubServerSocket = serverSocket;
    }

    /**
     * Return the predefined ServerSocket instance.
     * @see org.mockftpserver.core.socket.ServerSocketFactory#createServerSocket(int)
     */
    public ServerSocket createServerSocket(int port) throws IOException {
        return stubServerSocket;
    }
}