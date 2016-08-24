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

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.ResourceBundle;

import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.Assert;
import org.mockftpserver.core.util.AssertFailedException;

/**
 * Composite CommandHandler that manages an internal list of CommandHandlers to which it delegates.
 * The internal CommandHandlers are maintained in an ordered list. Starting with the first 
 * CommandHandler in the list, each invocation of this composite handler will invoke (delegate to) 
 * the current internal CommandHander. Then it moves on the next CommandHandler in the internal list.  
 * <p>
 * The following example replaces the CWD CommandHandler with a <code>SimpleCompositeCommandHandler</code>. 
 * The first invocation of the CWD command will fail (reply code 500). The seconds will succeed.
 * <pre><code>
 * 
 * StubFtpServer stubFtpServer = new StubFtpServer();
 * 
 * CommandHandler commandHandler1 = new StaticReplyCommandHandler(500);
 * CommandHandler commandHandler2 = new CwdCommandHandler();
 * 
 * SimpleCompositeCommandHandler simpleCompositeCommandHandler = new SimpleCompositeCommandHandler();
 * simpleCompositeCommandHandler.addCommandHandler(commandHandler1);
 * simpleCompositeCommandHandler.addCommandHandler(commandHandler2);
 * 
 * stubFtpServer.setCommandHandler("CWD", simpleCompositeCommandHandler);
 * </code></pre>
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class SimpleCompositeCommandHandler implements CommandHandler, ReplyTextBundleAware {

    private List commandHandlers = new ArrayList();
    private int invocationIndex = 0;
    
    /**
     * Add a CommandHandler to the internal list of handlers.
     * 
     * @param commandHandler - the CommandHandler
     *      
     * @throws AssertFailedException - if the commandHandler is null      
     */
    public void addCommandHandler(CommandHandler commandHandler) {
        Assert.notNull(commandHandler, "commandHandler");
        commandHandlers.add(commandHandler);
    }
    
    /**
     * Set the List of CommandHandlers to which to delegate. This replaces any CommandHandlers that
     * have been defined previously.
     * @param commandHandlers - the complete List of CommandHandlers to which invocations are delegated
     */
    public void setCommandHandlers(List commandHandlers) {
        Assert.notNull(commandHandlers, "commandHandlers");
        this.commandHandlers = new ArrayList(commandHandlers);
    }
    
    /**
     * Return the CommandHandler corresponding to the specified invocation index. In other words, return
     * the CommandHandler instance to which the Nth {@link #handleCommand(Command, Session)} has been or will
     * be delegated (where N=index).
     * @param index - the index of the desired invocation (zero-based).
     * @return the CommandHandler
     * 
     * @throws AssertFailedException - if no CommandHandler is defined for the index or the index is not valid
     */
    public CommandHandler getCommandHandler(int index) {
        Assert.isTrue(index < commandHandlers.size(), "No CommandHandler defined for index " + index);
        Assert.isTrue(index >= 0, "The index cannot be less than zero: " + index);
        return (CommandHandler) commandHandlers.get(index);
    }
    
    /**
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session)
     */
    public void handleCommand(Command command, Session session) throws Exception {
        Assert.notNull(command, "command");
        Assert.notNull(session, "session");
        Assert.isTrue(commandHandlers.size() > invocationIndex, "No CommandHandler defined for invocation #" + invocationIndex);
        
        CommandHandler commandHandler = (CommandHandler) commandHandlers.get(invocationIndex);
        invocationIndex++;
        commandHandler.handleCommand(command, session);
    }

    /**
     * Returns null. This is a composite, and has no reply text bundle.
     * 
     * @see org.mockftpserver.core.command.ReplyTextBundleAware#getReplyTextBundle()
     */
    public ResourceBundle getReplyTextBundle() {
        return null;
    }

    /**
     * Call <code>setReplyTextBundle()</code> on each of the command handlers within the internal list.
     * 
     * @see org.mockftpserver.core.command.ReplyTextBundleAware#setReplyTextBundle(java.util.ResourceBundle)
     */
    public void setReplyTextBundle(ResourceBundle replyTextBundle) {
        for (Iterator iter = commandHandlers.iterator(); iter.hasNext();) {
            CommandHandler commandHandler = (CommandHandler) iter.next();
            ReplyTextBundleUtil.setReplyTextBundleIfAppropriate(commandHandler, replyTextBundle);
        }
    }

}
