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

import org.mockftpserver.core.CommandSyntaxException;
import org.mockftpserver.core.MockFtpServerException;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.List;

/**
 * Utility class for parsing port values from command arguments.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class PortParser {

    /**
     * Private constructor. All methods are static.
     */
    private PortParser() {
    }

    /**
     * Parse a 32-bit IP address from the String[] of FTP command parameters.
     *
     * @param parameters - the String[] of command parameters. It is the concatenation
     *                   of a 32-bit internet host address and a 16-bit TCP port address. This address
     *                   information is broken into 8-bit fields and the value of each field is encoded
     *                   as a separate parameter whose value is a decimal number (in character string
     *                   representation).  Thus, the six parameters for the port command would be:
     *                   h1,h2,h3,h4,p1,p2
     *                   where h1 is the high order 8 bits of the internet host address, and p1 is the
     *                   high order 8 bits of the port number.
     * @return the InetAddres representing the host parsed from the parameters
     * @throws org.mockftpserver.core.util.AssertFailedException
     *                               - if parameters is null or contains an insufficient number of elements
     * @throws NumberFormatException - if one of the parameters does not contain a parsable integer
     */
    public static InetAddress parseHost(String[] parameters) {
        verifySufficientParameters(parameters);

        byte host1 = parseByte(parameters[0]);
        byte host2 = parseByte(parameters[1]);
        byte host3 = parseByte(parameters[2]);
        byte host4 = parseByte(parameters[3]);

        byte[] address = {host1, host2, host3, host4};
        InetAddress inetAddress = null;
        try {
            inetAddress = InetAddress.getByAddress(address);
        }
        catch (UnknownHostException e) {
            throw new MockFtpServerException("Error parsing host", e);
        }

        return inetAddress;
    }

    /**
     * Parse a 16-bit port number from the String[] of FTP command parameters.
     *
     * @param parameters - the String[] of command parameters. It is the concatenation
     *                   of a 32-bit internet host address and a 16-bit TCP port address. This address
     *                   information is broken into 8-bit fields and the value of each field is encoded
     *                   as a separate parameter whose value is a decimal number (in character string
     *                   representation).  Thus, the six parameters for the port command would be:
     *                   h1,h2,h3,h4,p1,p2
     *                   where h1 is the high order 8 bits of the internet host address, and p1 is the
     *                   high order 8 bits of the port number.
     * @return the port number parsed from the parameters
     * @throws org.mockftpserver.core.util.AssertFailedException
     *                               - if parameters is null or contains an insufficient number of elements
     * @throws NumberFormatException - if one of the parameters does not contain a parsable integer
     */
    public static int parsePortNumber(String[] parameters) {
        verifySufficientParameters(parameters);

        int port1 = Integer.parseInt(parameters[4]);
        int port2 = Integer.parseInt(parameters[5]);
        int port = (port1 << 8) + port2;

        return port;
    }

    /**
     * Verify that the parameters is not null and contains the required number of elements
     *
     * @param parameters - the String[] of command parameters
     * @throws CommandSyntaxException - if parameters is null or contains an insufficient number of elements
     */
    private static void verifySufficientParameters(String[] parameters) {
        if (parameters == null || parameters.length < 6) {
            List parms = parameters == null ? null : Arrays.asList(parameters);
            throw new CommandSyntaxException("The PORT command must contain least be 6 parameters: " + parms);
        }
    }

    /**
     * Convert the InetAddess and port number to a comma-delimited list of byte values,
     * suitable for the response String from the PASV command.
     *
     * @param host - the InetAddress
     * @param port - the port number
     * @return the comma-delimited list of byte values, e.g., "196,168,44,55,23,77"
     */
    public static String convertHostAndPortToCommaDelimitedBytes(InetAddress host, int port) {
        StringBuffer buffer = new StringBuffer();
        byte[] address = host.getAddress();
        for (int i = 0; i < address.length; i++) {
            int positiveValue = (address[i] >= 0) ? address[i] : 256 + address[i];
            buffer.append(positiveValue);
            buffer.append(",");
        }
        int p1 = port >> 8;
        int p2 = port % 256;
        buffer.append(String.valueOf(p1));
        buffer.append(",");
        buffer.append(String.valueOf(p2));
        return buffer.toString();
    }

    /**
     * Parse the specified String as an unsigned decimal byte value (i.e., 0..255). We can't just use
     * Byte.parseByte(string) because that parses the string as a signed byte.
     *
     * @param string - the String containing the decimal byte representation to be parsed
     * @return the byte value
     */
    private static byte parseByte(String string) {
        return (byte) (0xFF & Short.parseShort(string));
    }

}