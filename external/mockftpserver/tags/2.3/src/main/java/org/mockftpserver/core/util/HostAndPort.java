/*
 * Copyright 2009 the original author or authors.
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
package org.mockftpserver.core.util;

import java.net.InetAddress;

/**
 * A data-only (transfer) object representing a host (InetAddress) and port number
 * that together uniquely identify an endpoint for a socket connection.
 *
 * This class contains two public properties: host (java.net.InetAddress) and port (int).
 *
 * @author Chris Mair
 * @version : $ - :  $
 */
public class HostAndPort {
    public InetAddress host;
    public int port;

    /**
     * Construct a new instance with the specified host and port
     * @param host - the InetAddress host
     * @param port - the port number
     */
    public HostAndPort(InetAddress host, int port) {
        this.host = host;
        this.port = port;
    }
}
