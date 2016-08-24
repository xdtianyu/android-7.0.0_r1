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
package org.mockftpserver.core.util

import org.mockftpserver.test.AbstractGroovyTestCase

/**
 * Tests for the IoUtil class
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
public class IoUtilTest extends AbstractGroovyTestCase {

    /**
     * Test the readBytes() method 
     */
    void testReadBytes() {
        final byte[] BYTES = "abc 123 %^&".getBytes()
        InputStream input = new ByteArrayInputStream(BYTES)
        assert IoUtil.readBytes(input) == BYTES
    }

    /**
     * Test the readBytes() method, passing in a null 
     */
    void testReadBytes_Null() {
        shouldFailWithMessageContaining("input") { IoUtil.readBytes(null) }
    }

}
