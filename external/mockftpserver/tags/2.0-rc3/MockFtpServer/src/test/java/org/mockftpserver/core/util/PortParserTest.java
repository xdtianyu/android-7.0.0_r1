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
package org.mockftpserver.core.util;

import org.apache.log4j.Logger;
import org.mockftpserver.core.CommandSyntaxException;
import org.mockftpserver.test.AbstractTest;

import java.net.InetAddress;
import java.net.UnknownHostException;

/**
 * Tests for the PortParser class
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class PortParserTest extends AbstractTest {

    private static final Logger LOG = Logger.getLogger(PortParserTest.class);
    private static final String[] PARAMETERS = new String[]{"192", "22", "250", "44", "1", "206"};
    private static final String[] PARAMETERS_INSUFFICIENT = new String[]{"7", "29", "99", "11", "77"};
    private static final int PORT = (1 << 8) + 206;
    private static final InetAddress HOST = inetAddress("192.22.250.44");

    /**
     * Test the parseHost() method
     *
     * @throws java.net.UnknownHostException
     */
    public void testParseHost() throws UnknownHostException {
        InetAddress host = PortParser.parseHost(PARAMETERS);
        assertEquals("InetAddress", HOST, host);
    }

    /**
     * Test the parsePortNumber() method
     */
    public void testParsePortNumber() {
        int portNumber = PortParser.parsePortNumber(PARAMETERS);
        assertEquals("portNumber", PORT, portNumber);
    }

    /**
     * Test the parseHost() method, passing in null
     *
     * @throws java.net.UnknownHostException
     */
    public void testParseHost_Null() throws UnknownHostException {
        try {
            PortParser.parseHost(null);
            fail("Expected CommandSyntaxException");
        }
        catch (CommandSyntaxException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test the parseHost() method, passing in a String[] with not enough parameters
     *
     * @throws java.net.UnknownHostException
     */
    public void testParseHost_InsufficientParameters() throws UnknownHostException {
        try {
            PortParser.parseHost(PARAMETERS_INSUFFICIENT);
            fail("Expected CommandSyntaxException");
        }
        catch (CommandSyntaxException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test the parsePortNumber() method, passing in null
     *
     * @throws java.net.UnknownHostException
     */
    public void testParsePortNumber_Null() throws UnknownHostException {
        try {
            PortParser.parsePortNumber(null);
            fail("Expected CommandSyntaxException");
        }
        catch (CommandSyntaxException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test the parsePortNumber() method, passing in a String[] with not enough parameters
     *
     * @throws java.net.UnknownHostException
     */
    public void testParsePortNumber_InsufficientParameters() throws UnknownHostException {
        try {
            PortParser.parsePortNumber(PARAMETERS_INSUFFICIENT);
            fail("Expected CommandSyntaxException");
        }
        catch (CommandSyntaxException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test convertHostAndPortToStringOfBytes() method
     */
    public void testConvertHostAndPortToStringOfBytes() throws UnknownHostException {
        int port = (23 << 8) + 77;
        InetAddress host = InetAddress.getByName("196.168.44.55");
        String result = PortParser.convertHostAndPortToCommaDelimitedBytes(host, port);
        LOG.info("result=" + result);
        assertEquals("result", "196,168,44,55,23,77", result);
    }

}