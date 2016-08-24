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

import org.apache.log4j.Logger;
import org.mockftpserver.core.CommandSyntaxException;
import org.mockftpserver.core.util.AssertFailedException;
import org.mockftpserver.test.AbstractTestCase;

import java.util.List;

/**
 * Tests for the Command class
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class CommandTest extends AbstractTestCase {

    private static final Logger LOG = Logger.getLogger(CommandTest.class);

    /**
     * Test the Command(String,String[]) constructor
     */
    public void testConstructor() {
        final String[] PARAMETERS = array("123");
        Command command = new Command("abc", PARAMETERS);
        assertEquals("name", "abc", command.getName());
        assertEquals("parameters", PARAMETERS, command.getParameters());
    }

    /**
     * Test the Command(String,List) constructor
     */
    public void testConstructor_List() {
        final List PARAMETERS_LIST = list("123");
        final String[] PARAMETERS_ARRAY = array("123");
        Command command = new Command("abc", PARAMETERS_LIST);
        assertEquals("name", "abc", command.getName());
        assertEquals("parameters String[]", PARAMETERS_ARRAY, command.getParameters());
    }

    /**
     * Test the Constructor method, passing in a null name
     */
    public void testConstructor_NullName() {
        try {
            new Command(null, EMPTY);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test the Constructor method, passing in a null parameters
     */
    public void testConstructor_NullParameters() {
        try {
            new Command("OK", (String[]) null);
            fail("Expected AssertFailedException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test the normalizeName() method
     */
    public void testNormalizeName() {
        assertEquals("XXX", "XXX", Command.normalizeName("XXX"));
        assertEquals("xxx", "XXX", Command.normalizeName("xxx"));
        assertEquals("Xxx", "XXX", Command.normalizeName("Xxx"));
    }

    /**
     * Test the getRequiredParameter method
     */
    public void testGetRequiredParameter() {
        Command command = new Command("abc", array("123", "456"));
        assertEquals("123", "123", command.getRequiredParameter(0));
        assertEquals("456", "456", command.getRequiredParameter(1));
    }

    /**
     * Test the getRequiredParameter method, when the index is not valid
     */
    public void testGetRequiredParameter_IndexNotValid() {
        Command command = new Command("abc", array("123", "456"));
        try {
            command.getRequiredParameter(2);
            fail("Expected CommandSyntaxException");
        }
        catch (CommandSyntaxException expected) {
            LOG.info("Expected: " + expected);
        }
    }

    /**
     * Test the getOptionalString method
     */
    public void testGetOptionalString() {
        Command command = new Command("abc", array("123", "456"));
        assertEquals("123", "123", command.getOptionalString(0));
        assertEquals("456", "456", command.getOptionalString(1));
        assertEquals("null", null, command.getOptionalString(2));
    }

    /**
     * Test the getParameter method
     */
    public void testGetParameter() {
        Command command = new Command("abc", array("123", "456"));
        assertEquals("123", "123", command.getParameter(0));
        assertEquals("456", "456", command.getParameter(1));
        assertEquals("null", null, command.getParameter(2));
    }

    /**
     * Test that a Command object is immutable, changing the original parameters passed in to the constructor
     */
    public void testImmutable_ChangeOriginalParameters() {
        final String[] PARAMETERS = {"a", "b", "c"};
        final Command COMMAND = new Command("command", PARAMETERS);
        PARAMETERS[2] = "xxx";
        assertEquals("parameters", COMMAND.getParameters(), new String[]{"a", "b", "c"});
    }

    /**
     * Test that a Command object is immutable, changing the parameters returned from getParameters
     */
    public void testImmutable_ChangeRetrievedParameters() {
        final String[] PARAMETERS = {"a", "b", "c"};
        final Command COMMAND = new Command("command", PARAMETERS);
        String[] parameters = COMMAND.getParameters();
        parameters[2] = "xxx";
        assertEquals("parameters", PARAMETERS, COMMAND.getParameters());
    }

    /**
     * Test the equals() method, and tests the hasCode() method implicitly
     *
     * @throws Exception
     */
    public void testEquals() throws Exception {
        final Command COMMAND1 = new Command("a", EMPTY);
        final Command COMMAND2 = new Command("a", EMPTY);
        final Command COMMAND3 = new Command("b", array("1"));
        final Command COMMAND4 = new Command("b", array("2"));
        final Command COMMAND5 = new Command("c", array("1"));
        _testEquals(COMMAND1, null, false);
        _testEquals(COMMAND1, COMMAND1, true);
        _testEquals(COMMAND1, COMMAND2, true);
        _testEquals(COMMAND1, COMMAND3, false);
        _testEquals(COMMAND3, COMMAND4, false);
        _testEquals(COMMAND3, COMMAND5, false);
    }

    /**
     * Test that command1 equals command2 if and only if expectedEqual is true
     *
     * @param command1      - the first command
     * @param command2      - the second command
     * @param expectedEqual - true if command1 is expected to equal command2
     */
    private void _testEquals(Command command1, Command command2, boolean expectedEqual) {
        assertEquals(command1.toString() + " and " + command2, expectedEqual, command1.equals(command2));
    }

}
