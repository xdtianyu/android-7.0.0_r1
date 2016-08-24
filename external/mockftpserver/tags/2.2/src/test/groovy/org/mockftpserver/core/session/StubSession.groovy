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

/**
 * Stub implementation of the         {@link Session}         interface for testing
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class StubSession implements Session {

    Map attributes = [:]
    private List sentReplies = []
    List sentData = []
    //byte[] dataToRead
    Object dataToRead
    boolean closed
    InetAddress clientDataHost
    int clientDataPort
    boolean dataConnectionOpen = false
    int switchToPassiveModeReturnValue
    boolean switchedToPassiveMode = false
    InetAddress serverHost

    /**
     * @see org.mockftpserver.core.session.Session#close()
     */
    public void close() {
        closed = true
    }

    /**
     * @see org.mockftpserver.core.session.Session#closeDataConnection()
     */
    public void closeDataConnection() {
        dataConnectionOpen = false
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
        return serverHost
    }

    /**
     * @see org.mockftpserver.core.session.Session#openDataConnection()
     */
    public void openDataConnection() {
        dataConnectionOpen = true
    }

    /**
     * @see org.mockftpserver.core.session.Session#readData()
     */
    public byte[] readData() {
        assert dataConnectionOpen, "The data connection must be OPEN"
        return dataToRead
    }

    /**
     * @see org.mockftpserver.core.session.Session#removeAttribute(java.lang.String)
     */
    public void removeAttribute(String name) {
        attributes.remove(name)
    }

    /**
     * @see org.mockftpserver.core.session.Session#sendData(byte [], int)
     */
    public void sendData(byte[] data, int numBytes) {
        assert dataConnectionOpen, "The data connection must be OPEN"
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
     * @see org.mockftpserver.core.session.Session#switchToPassiveMode()
     */
    public int switchToPassiveMode() {
        switchedToPassiveMode = true
        return switchToPassiveModeReturnValue
    }

    /**
     * @see java.lang.Runnable#run()
     */
    public void run() {

    }

    //-------------------------------------------------------------------------
    // Stub-specific API - Helper methods not part of Session interface
    //-------------------------------------------------------------------------

    /**
     * @return the reply code for the session reply at the specified index
     */
    int getReplyCode(int replyIndex) {
        return getReply(replyIndex)[0]
    }

    /**
     * @return the reply message for the session reply at the specified index
     */
    String getReplyMessage(int replyIndex) {
        return getReply(replyIndex)[1]
    }

    /**
     * @return the String representation of this object, including property names and values of interest
     */
    String toString() {
        "StubSession[sentReplies=$sentReplies  sentData=$sentData  attributes=$attributes  closed=$closed  " +
                "clientDataHost=$clientDataHost  clientDataPort=$clientDataPort]"
    }

    //-------------------------------------------------------------------------
    // Internal Helper Methods
    //-------------------------------------------------------------------------

    private List getReply(int replyIndex) {
        def reply = sentReplies[replyIndex]
        assert reply, "No reply for index [$replyIndex] sent for ${this}"
        return reply
    }

}
