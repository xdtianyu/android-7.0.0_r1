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
package org.mockftpserver.core.util;

/**
 * Exception that indicates that a runtime assertion from the 
 * {@link org.mockftpserver.core.util.Assert} has failed. 
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class AssertFailedException extends RuntimeException {

    /**
     * Create a new instance for the specified message
     * @param message - the exception message
     */
    public AssertFailedException(final String message) {
        super(message);
    }

}
