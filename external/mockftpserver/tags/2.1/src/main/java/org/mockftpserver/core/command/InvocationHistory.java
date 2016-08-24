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
package org.mockftpserver.core.command;

/**
 * Interface for an object that can retrieve and clear the history of InvocationRecords 
 * for a command handler.
 * 
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
public interface InvocationHistory {

    /**
     * @return the number of invocation records stored for this command handler instance
     */
    public int numberOfInvocations();

    /**
     * Return the InvocationRecord representing the command invoction data for the nth invocation
     * for this command handler instance. One InvocationRecord should be stored for each invocation
     * of the CommandHandler.
     * 
     * @param index - the index of the invocation record to return. The first record is at index zero.
     * @return the InvocationRecord for the specified index
     * 
     * @throws AssertFailedException - if there is no invocation record corresponding to the specified index     */
    public InvocationRecord getInvocation(int index);

    /**
     * Clear out the invocation history for this CommandHandler. After invoking this method, the
     * <code>numberOfInvocations()</code> method will return zero.
     */
    public void clearInvocations();

}