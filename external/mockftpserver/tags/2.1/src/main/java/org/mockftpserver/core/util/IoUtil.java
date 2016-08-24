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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Contains static I/O-related utility methods.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class IoUtil {

    /**
     * Read the contents of the InputStream and return as a byte[].
     *
     * @param input - the InputStream to read
     * @return the contents of the InputStream as a byte[]
     * @throws AssertFailedException - if the InputStream is null
     * @throws java.io.IOException   - if an error occurs reading the bytes
     */
    public static byte[] readBytes(InputStream input) throws IOException {
        Assert.notNull(input, "input");
        ByteArrayOutputStream outBytes = new ByteArrayOutputStream();

        try {
            while (true) {
                int b = input.read();
                if (b == -1) {
                    break;
                }
                outBytes.write(b);
            }
        }
        finally {
            input.close();
        }
        return outBytes.toByteArray();
    }

    /**
     * Private constructor to prevent instantiation. All members are static.
     */
    private IoUtil() {
    }

}
