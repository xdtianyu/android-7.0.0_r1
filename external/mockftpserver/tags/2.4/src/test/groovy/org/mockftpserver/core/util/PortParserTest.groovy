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
package org.mockftpserver.core.util

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.mockftpserver.core.CommandSyntaxException
import org.mockftpserver.test.AbstractGroovyTestCase

/**
 * Tests for the PortParser class
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
class PortParserTest extends AbstractGroovyTestCase {

    static final Logger LOG = LoggerFactory.getLogger(PortParserTest.class)
    static final String[] PARAMETERS = ["192", "22", "250", "44", "1", "206"]
    static final String[] PARAMETERS_INSUFFICIENT = ["7", "29", "99", "11", "77"]
    static final int PORT = (1 << 8) + 206
    static final InetAddress HOST = inetAddress("192.22.250.44")

    static final PARAMETER_IPV4 = "|1|132.235.1.2|6275|"
    static final HOST_IPV4 = InetAddress.getByName("132.235.1.2")
    static final PARAMETER_IPV6 = "|2|1080::8:800:200C:417A|6275|"
    static final HOST_IPV6 = InetAddress.getByName("1080::8:800:200C:417A")
    static final E_PORT = 6275

    void testParseExtendedAddressHostAndPort_IPv4() {
        def client = PortParser.parseExtendedAddressHostAndPort(PARAMETER_IPV4)
        assert client.host == HOST_IPV4
        assert client.port == E_PORT
    }

    void testParseExtendedAddressHostAndPort_IPv6() {
        def client = PortParser.parseExtendedAddressHostAndPort(PARAMETER_IPV6)
        assert client.host == HOST_IPV6
        assert client.port == E_PORT
    }

    void testParseExtendedAddressHostAndPort_IPv6_CustomDelimiter() {
        def client = PortParser.parseExtendedAddressHostAndPort("@2@1080::8:800:200C:417A@6275@")
        assert client.host == HOST_IPV6
        assert client.port == E_PORT
    }

    void testParseExtendedAddressHostAndPort_IllegalParameterFormat() {
        final PARM = 'abcdef'
        shouldFail(CommandSyntaxException) { PortParser.parseExtendedAddressHostAndPort(PARM) }
    }

    void testParseExtendedAddressHostAndPort_PortMissing() {
        final PARM = '|1|132.235.1.2|'
        shouldFail(CommandSyntaxException) { PortParser.parseExtendedAddressHostAndPort(PARM) }
    }

    void testParseExtendedAddressHostAndPort_IllegalHostName() {
        final PARM = '|1|132.@|6275|'
        shouldFail(CommandSyntaxException) { PortParser.parseExtendedAddressHostAndPort(PARM) }
    }

    void testParseExtendedAddressHostAndPort_Null() {
        shouldFail(CommandSyntaxException) { PortParser.parseExtendedAddressHostAndPort(null) }
    }

    void testParseExtendedAddressHostAndPort_Empty() {
        shouldFail(CommandSyntaxException) { PortParser.parseExtendedAddressHostAndPort('') }
    }

    void testParseHostAndPort() {
        def client = PortParser.parseHostAndPort(PARAMETERS)
        assert client.host == HOST
        assert client.port == PORT
    }

    void testParseHostAndPort_Null() {
        shouldFail(CommandSyntaxException) { PortParser.parseHostAndPort(null) }
    }

    void testParseHostAndPort_InsufficientParameters() throws UnknownHostException {
        shouldFail(CommandSyntaxException) { PortParser.parseHostAndPort(PARAMETERS_INSUFFICIENT) }
    }

    void testConvertHostAndPortToStringOfBytes() {
        int port = (23 << 8) + 77
        InetAddress host = InetAddress.getByName("196.168.44.55")
        String result = PortParser.convertHostAndPortToCommaDelimitedBytes(host, port)
        LOG.info("result=" + result)
        assertEquals("result", "196,168,44,55,23,77", result)
    }

}