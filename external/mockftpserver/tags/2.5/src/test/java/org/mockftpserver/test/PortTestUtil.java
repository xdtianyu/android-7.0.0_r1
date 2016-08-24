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

package org.mockftpserver.test;

/**
 * Contains static test utility method to determine FTP server port number to use for tests  
 * 
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
public final class PortTestUtil {

    private static final int DEFAULT_SERVER_CONTROL_PORT = 21;
    private static final String FTP_SERVER_PORT_PROPERTY = "ftp.server.port";
    
    /**
     * Return the port number to use for the FTP server control port. If the "ftp.server.port"
     * system property is defined, then use that value (converted to an integer), otherwise
     * return the default port number of 21.
     * 
     * @return the port number to use for the FTP server control port
     */
    public static int getFtpServerControlPort() {
        String systemProperty = System.getProperty(FTP_SERVER_PORT_PROPERTY);
        return (systemProperty == null) ? DEFAULT_SERVER_CONTROL_PORT : Integer.parseInt(systemProperty);
    }
    
}
