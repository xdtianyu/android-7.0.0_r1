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
package org.mockftpserver.core.session;

import java.net.InetAddress;
import java.util.Set;

/**
 * Represents an FTP session state and behavior
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public interface Session extends Runnable {

    /**
     * Close the session, closing the underlying sockets
     */
    public void close();

    /**
     * Send the specified reply code and text across the control connection.
     * 
     * @param replyCode - the reply code
     * @param replyText - the reply text to send; may be null
     */
    public void sendReply(int replyCode, String replyText);

    /**
     * Open the data connection, attaching to the predefined port number on the client
     */
    public void openDataConnection();

    /**
     * Close the data connection
     */
    public void closeDataConnection();

    /**
     * Switch to passive mode
     * @return the local port to be connected to by clients for data transfers
     */
    public int switchToPassiveMode();
    
    /**
     * Write the specified data using the data connection
     * 
     * @param data - the data to write
     * @param numBytes - the number of bytes from data to send
     */
    public void sendData(byte[] data, int numBytes);

    /**
     * Read data from the client across the data connection
     * 
     * @return the data that was read
     */
    public byte[] readData();

    /**
     * Read and return (up to) numBytes of data from the client across the data connection
     *
     * @return the data that was read; the byte[] will be up to numBytes bytes long
     */
    public byte[] readData(int numBytes);

    /**
     * Return the InetAddress representing the client host for this session
     * @return the client host
     */
    public InetAddress getClientHost();
    
    /**
     * Return the InetAddress representing the server host for this session
     * @return the server host
     */
    public InetAddress getServerHost();
    
    /**
     * @param clientHost - the client host for the data connection
     */
    public void setClientDataHost(InetAddress clientHost);

    /**
     * @param clientDataPort - the port number on the client side for the data connection
     */
    public void setClientDataPort(int clientDataPort);

    /**
     * Return the attribute value for the specified name. Return null if no attribute value
     * exists for that name or if the attribute value is null.
     * @param name - the attribute name; may not be null
     * @return the value of the attribute stored under name; may be null
     * @throws AssertFailedException - if name is null
     */
    public Object getAttribute(String name);
    
    /**
     * Store the value under the specified attribute name.
     * @param name - the attribute name; may not be null
     * @param value - the attribute value; may be null
     * @throws AssertFailedException - if name is null
     */
    public void setAttribute(String name, Object value);
    
    /**
     * Remove the attribute value for the specified name. Do nothing if no attribute
     * value is stored for the specified name.
     * @param name - the attribute name; may not be null
     * @throws AssertFailedException - if name is null
     */
    public void removeAttribute(String name);

    /**
     * Return the Set of names under which attributes have been stored on this session.
     * Returns an empty Set if no attribute values are stored.
     * @return the Set of attribute names
     */
    public Set getAttributeNames();
    
}