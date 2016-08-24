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
package org.mockftpserver.core;

/**
 * Represents an error specific to the MockFtpServer project.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public class MockFtpServerException extends RuntimeException {

    /**
     * Create a new instance with the specified detail message and no cause
     * @param message - the exception detail message
     */
    public MockFtpServerException(String message) {
        super(message);
    }

    /**
     * Create a new instance with the specified detail message and no cause
     * @param cause - the Throwable cause
     */
    public MockFtpServerException(Throwable cause) {
        super(cause);
    }

    /**
     * Create a new instance with the specified detail message and cause
     * @param message - the exception detail message
     * @param cause - the Throwable cause
     */
    public MockFtpServerException(String message, Throwable cause) {
        super(message, cause);
    }

}
