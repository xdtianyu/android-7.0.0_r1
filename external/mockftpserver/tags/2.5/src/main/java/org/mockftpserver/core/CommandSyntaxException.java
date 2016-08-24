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
 * Represents an error indicating that a server command has been received that
 * has invalid syntax. For instance, the command may be missing a required parameter.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class CommandSyntaxException extends MockFtpServerException {

    /**
     * @param message
     */
    public CommandSyntaxException(String message) {
        super(message);
    }

    /**
     * @param cause
     */
    public CommandSyntaxException(Throwable cause) {
        super(cause);
    }

    /**
     * @param message
     * @param cause
     */
    public CommandSyntaxException(String message, Throwable cause) {
        super(message, cause);
    }

}
