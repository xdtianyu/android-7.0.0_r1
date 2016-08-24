/*
 * Copyright 2008 the original author or authors.
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
package org.mockftpserver.core.session

import java.net.InetAddress
import java.util.Set

/**
 * Stub implementation of the {@link Session} interface for testing
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
class StubSession implements Session {

     Map attributes = [:]
     List sentReplies = [ ]
     List sentData = [ ]
     byte[] dataToRead
     
    /**
     * @see org.mockftpserver.core.session.Session#close()
     */
    public void close() {

    }

    /**
     * @see org.mockftpserver.core.session.Session#closeDataConnection()
     */
    public void closeDataConnection() {

    }

    /**
     * @see org.mockftpserver.core.session.Session#getAttribute(java.lang.String)
     */
    public Object getAttribute(String name) {
        return attributes[name]
    }

    /**
     * @see org.mockftpserver.core.session.Session#getAttributeNames()
     */
    public Set getAttributeNames() {
        return attributes.keySet()
    }

    /**
     * @see org.mockftpserver.core.session.Session#getClientHost()
     */
    public InetAddress getClientHost() {
        return null
    }

    /**
     * @see org.mockftpserver.core.session.Session#getServerHost()
     */
    public InetAddress getServerHost() {
        return null
    }

    /**
     * @see org.mockftpserver.core.session.Session#openDataConnection()
     */
    public void openDataConnection() {

    }

    /**
     * @see org.mockftpserver.core.session.Session#readData()
     */
    public byte[] readData() {
        return dataToRead
    }

    /**
     * @see org.mockftpserver.core.session.Session#removeAttribute(java.lang.String)
     */
    public void removeAttribute(String name) {
        attributes.remove(name)
    }

    /**
     * @see org.mockftpserver.core.session.Session#sendData(byte[], int)
     */
    public void sendData(byte[] data, int numBytes) {
        sentData << new String(data, 0, numBytes)
    }

    /**
     * @see org.mockftpserver.core.session.Session#sendReply(int, java.lang.String)
     */
    public void sendReply(int replyCode, String replyText) {
        sentReplies << [replyCode, replyText]
    }

    /**
     * @see org.mockftpserver.core.session.Session#setAttribute(java.lang.String, java.lang.Object)
     */
    public void setAttribute(String name, Object value) {
        attributes[name] = value
    }

    /**
     * @see org.mockftpserver.core.session.Session#setClientDataHost(java.net.InetAddress)
     */
    public void setClientDataHost(InetAddress clientHost) {

    }

    /**
     * @see org.mockftpserver.core.session.Session#setClientDataPort(int)
     */
    public void setClientDataPort(int clientDataPort) {

    }

    /**
     * @see org.mockftpserver.core.session.Session#switchToPassiveMode()
     */
    public int switchToPassiveMode() {
        return 0
    }

    /**
     * @see java.lang.Runnable#run()
     */
    public void run() {

    }

    //-------------------------------------------------------------------------
    // Stub-specific API - Helper methods not part of Session interface
    //-------------------------------------------------------------------------
    
    String toString() {
        "StubSession[sentReplies=$sentReplies  sentData=$sentData]"
    }
    
}
