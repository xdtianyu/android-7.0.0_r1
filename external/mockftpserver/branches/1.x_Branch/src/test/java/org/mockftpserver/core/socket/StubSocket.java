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
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;

/**
 * Test (fake) subclass of Socket that performs no network access and allows setting the 
 * inputStream and OutputStream for the socket.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class StubSocket extends Socket {

    private InetAddress inetAddress;
    private InetAddress localAddress;
    private InputStream inputStream;
    private OutputStream outputStream;
    
    /**
     * Construct a new instance using the specified InputStream and OutputStream
     * @param inputStream - the InputStream to use
     * @param outputStream - the OutputStream to use
     */
    public StubSocket(InputStream inputStream, OutputStream outputStream) {
        this(null, inputStream, outputStream);
    }
    
    /**
     * Construct a new instance using the specified host, InputStream and OutputStream
     * @param inetAddress - the InetAddress for this socket
     * @param inputStream - the InputStream to use
     * @param outputStream - the OutputStream to use
     */
    public StubSocket(InetAddress inetAddress, InputStream inputStream, OutputStream outputStream) {
        this.inetAddress = inetAddress;
        this.inputStream = inputStream;
        this.outputStream = outputStream;
    }
    
    /**
     * Override the superclass implementation. If the local inetAddress is not null, 
     * return that. Otherwise return super.getInetAddress().
     * @see java.net.Socket#getInetAddress()
     */
    public InetAddress getInetAddress() {
        return (inetAddress != null) ? inetAddress : super.getInetAddress();
    }
    
    /**
     * Override the superclass implementation. If the local localAddress is not
     * null, return that. Otherwise return super.getLocalAddress();
     * @see java.net.Socket#getLocalAddress()
     */
    public InetAddress getLocalAddress() {
        return (localAddress != null) ? localAddress : super.getLocalAddress();
    }
    
    /**
     * Override the superclass implementation to provide the predefined InputStream
     * @see java.net.Socket#getInputStream()
     */
    public InputStream getInputStream() throws IOException {
        return inputStream;
    }
    
    /**
     * Override the superclass implementation to provide the predefined OutputStream
     * @see java.net.Socket#getOutputStream()
     */
    public OutputStream getOutputStream() throws IOException {
        return outputStream;
    }
    
    //-------------------------------------------------------------------------
    // Test-specific helper methods
    //-------------------------------------------------------------------------

    public void _setLocalAddress(InetAddress localAddress) {
        this.localAddress = localAddress;
    }
}
