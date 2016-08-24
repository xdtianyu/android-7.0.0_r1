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
package org.mockftpserver.core;

/**
 * Represents an error indicating that the current user has not yet logged in, but
 * is required to in order to invoke the requested command.
 *
 * @author Chris Mair
 * @version $Revision: 51 $ - $Date: 2008-05-09 22:16:32 -0400 (Fri, 09 May 2008) $
 */
public class NotLoggedInException extends MockFtpServerException {

    /**
     * @param message
     */
    public NotLoggedInException(String message) {
        super(message);
    }

    /**
     * @param cause
     */
    public NotLoggedInException(Throwable cause) {
        super(cause);
    }

    /**
     * @param message
     * @param cause
     */
    public NotLoggedInException(String message, Throwable cause) {
        super(message, cause);
    }

}
