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

import java.util.ResourceBundle;

/**
 * Interface for objects that allow getting and setting a reply text ResourceBundle. This
 * interface is implemented by CommandHandlers so that the StubFtpServer can automatically
 * set the default reply text ResourceBundle for the CommandHandler.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public interface ReplyTextBundleAware {

    /**
     * Return the ResourceBundle containing the reply text messages
     * @return the replyTextBundle
     */
    public ResourceBundle getReplyTextBundle();

    /**
     * Set the ResourceBundle containing the reply text messages
     * @param replyTextBundle - the replyTextBundle to set
     */
    public void setReplyTextBundle(ResourceBundle replyTextBundle);

}