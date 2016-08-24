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
package org.mockftpserver.fake;

import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.CommandNames;
import org.mockftpserver.core.server.AbstractFtpServer;
import org.mockftpserver.fake.command.*;
import org.mockftpserver.fake.filesystem.FileSystem;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * <b>FakeFtpServer</b> is the top-level class for a "fake" implementation of an FTP Server,
 * suitable for testing FTP client code or standing in for a live FTP server.
 * <p/>
 * <b>FakeFtpServer</b> provides a high-level abstraction for an FTP Server and is suitable
 * for most testing and simulation scenarios. You define a filesystem (internal, in-memory) containing
 * an arbitrary set of files and directories. These files and directories can (optionally) have
 * associated access permissions. You also configure a set of one or more user accounts that
 * control which users can login to the FTP server, and their home (default) directories. The
 * user account is also used when assigning file and directory ownership for new files.
 * <p> <b>FakeFtpServer</b> processes FTP client requests and responds with reply codes and
 * reply messages consistent with its configuration and the contents of its internal filesystem,
 * including file and directory permissions, if they have been configured.
 * <p/>
 * <b>FakeFtpServer</b> can be fully configured programmatically or within the
 * <b>Spring Framework</b> or other dependency-injection container.
 * <p/>
 * In general the steps for setting up and starting the <b>FakeFtpServer</b> are:
 * <ol>
 * <li>Create a new <b>FakeFtpServer</b> instance, and optionally set the server control port.</li>
 * <li>Create and configure a <b>FileSystem</b>, and attach to the <b>FakeFtpServer</b> instance.</li>
 * <li>Create and configure one or more <b>UserAccount</b> objects and attach to the <b>FakeFtpServer</b> instance.</li>
 * </ol>
 * <h4>Example Code</h4>
 * <pre><code>
 * FileSystem fileSystem = new WindowsFakeFileSystem();
 * DirectoryEntry directoryEntry1 = new DirectoryEntry("c:\\");
 * directoryEntry1.setPermissions(new Permissions("rwxrwx---"));
 * directoryEntry1.setOwner("joe");
 * directoryEntry1.setGroup("dev");
 * <p/>
 * DirectoryEntry directoryEntry2 = new DirectoryEntry("c:\\data");
 * directoryEntry2.setPermissions(Permissions.ALL);
 * directoryEntry2.setOwner("joe");
 * directoryEntry2.setGroup("dev");
 * <p/>
 * FileEntry fileEntry1 = new FileEntry("c:\\data\\file1.txt", "abcdef 1234567890");
 * fileEntry1.setPermissionsFromString("rw-rw-rw-");
 * fileEntry1.setOwner("joe");
 * fileEntry1.setGroup("dev");
 * <p/>
 * FileEntry fileEntry2 = new FileEntry("c:\\data\\run.exe");
 * fileEntry2.setPermissionsFromString("rwxrwx---");
 * fileEntry2.setOwner("mary");
 * fileEntry2.setGroup("dev");
 * <p/>
 * fileSystem.add(directoryEntry1);
 * fileSystem.add(directoryEntry2);
 * fileSystem.add(fileEntry1);
 * fileSystem.add(fileEntry2);
 * <p/>
 * FakeFtpServer fakeFtpServer = new FakeFtpServer();
 * fakeFtpServer.setFileSystem(fileSystem);
 * <p/>
 * UserAccount userAccount = new UserAccount();
 * userAccount.setUsername("joe");
 * userAccount.setPassword("joe123");
 * userAccount.setHomeDirectory("c:\\");
 * List userAccounts = Collections.singletonList(userAccount);
 * fakeFtpServer.setUserAccounts(userAccounts);
 * <p/>
 * fakeFtpServer.start();
 * </code></pre>
 * <p/>
 * <h4>FTP Command Reply Text ResourceBundle</h4>
 * <p/>
 * The default text asociated with each FTP command reply code is contained within the
 * "ReplyText.properties" ResourceBundle file. You can customize these messages by providing a
 * locale-specific ResourceBundle file on the CLASSPATH, according to the normal lookup rules of
 * the ResourceBundle class (e.g., "ReplyText_de.properties"). Alternatively, you can
 * completely replace the ResourceBundle file by calling the calling the
 * {@link #setReplyTextBaseName(String)} method.
 *
 * @author Chris Mair
 * @version $Revision: 143 $ - $Date: 2008-10-31 21:07:23 -0400 (Fri, 31 Oct 2008) $
 */
public class FakeFtpServer extends AbstractFtpServer implements ServerConfiguration {

    private FileSystem fileSystem;
    private String systemName = "WINDOWS";
    private Map helpText = new HashMap();
    private Map userAccounts = new HashMap();

    public FileSystem getFileSystem() {
        return fileSystem;
    }

    public void setFileSystem(FileSystem fileSystem) {
        this.fileSystem = fileSystem;
    }

    public String getSystemName() {
        return systemName;
    }

    public void setSystemName(String systemName) {
        this.systemName = systemName;
    }

    public Map getHelpText() {
        return helpText;
    }

    public void setHelpText(Map helpText) {
        this.helpText = helpText;
    }

    public FakeFtpServer() {
        setCommandHandler(CommandNames.ACCT, new AcctCommandHandler());
        setCommandHandler(CommandNames.ABOR, new AborCommandHandler());
        setCommandHandler(CommandNames.ALLO, new AlloCommandHandler());
        setCommandHandler(CommandNames.APPE, new AppeCommandHandler());
        setCommandHandler(CommandNames.CONNECT, new ConnectCommandHandler());
        setCommandHandler(CommandNames.CWD, new CwdCommandHandler());
        setCommandHandler(CommandNames.CDUP, new CdupCommandHandler());
        setCommandHandler(CommandNames.DELE, new DeleCommandHandler());
        setCommandHandler(CommandNames.HELP, new HelpCommandHandler());
        setCommandHandler(CommandNames.LIST, new ListCommandHandler());
        setCommandHandler(CommandNames.MKD, new MkdCommandHandler());
        setCommandHandler(CommandNames.MODE, new ModeCommandHandler());
        setCommandHandler(CommandNames.NLST, new NlstCommandHandler());
        setCommandHandler(CommandNames.NOOP, new NoopCommandHandler());
        setCommandHandler(CommandNames.PASS, new PassCommandHandler());
        setCommandHandler(CommandNames.PASV, new PasvCommandHandler());
        setCommandHandler(CommandNames.PWD, new PwdCommandHandler());
        setCommandHandler(CommandNames.PORT, new PortCommandHandler());
        setCommandHandler(CommandNames.QUIT, new QuitCommandHandler());
        setCommandHandler(CommandNames.REIN, new ReinCommandHandler());
        setCommandHandler(CommandNames.REST, new RestCommandHandler());
        setCommandHandler(CommandNames.RETR, new RetrCommandHandler());
        setCommandHandler(CommandNames.RMD, new RmdCommandHandler());
        setCommandHandler(CommandNames.RNFR, new RnfrCommandHandler());
        setCommandHandler(CommandNames.RNTO, new RntoCommandHandler());
        setCommandHandler(CommandNames.SITE, new SiteCommandHandler());
        setCommandHandler(CommandNames.STOR, new StorCommandHandler());
        setCommandHandler(CommandNames.STOU, new StouCommandHandler());
        setCommandHandler(CommandNames.STRU, new StruCommandHandler());
        setCommandHandler(CommandNames.SYST, new SystCommandHandler());
        setCommandHandler(CommandNames.TYPE, new TypeCommandHandler());
        setCommandHandler(CommandNames.USER, new UserCommandHandler());
        setCommandHandler(CommandNames.XPWD, new PwdCommandHandler());
    }

    /**
     * Initialize a CommandHandler that has been registered to this server. If the CommandHandler implements
     * the <code>ServerConfigurationAware</code> interface, then set its <code>ServerConfiguration</code>
     * property to <code>this</code>.
     *
     * @param commandHandler - the CommandHandler to initialize
     */
    protected void initializeCommandHandler(CommandHandler commandHandler) {
        if (commandHandler instanceof ServerConfigurationAware) {
            ServerConfigurationAware sca = (ServerConfigurationAware) commandHandler;
            sca.setServerConfiguration(this);
        }
    }

    /**
     * @return the {@link UserAccount}        configured for this server for the specified user name
     */
    public UserAccount getUserAccount(String username) {
        return (UserAccount) userAccounts.get(username);
    }

    /**
     * Return the help text for a command or the default help text if no command name is specified
     *
     * @param name - the command name; may be empty or null to indicate  a request for the default help text
     * @return the help text for the named command or the default help text if no name is supplied
     */
    public String getHelpText(String name) {
        String key = name == null ? "" : name;
        return (String) helpText.get(key);
    }

    /**
     * Add a single UserAccount. If an account with the same <code>username</code> already exists,
     * it will be replaced.
     *
     * @param userAccount - the UserAccount to add
     */
    public void addUserAccount(UserAccount userAccount) {
        userAccounts.put(userAccount.getUsername(), userAccount);
    }

    /**
     * Add the UserAccount objects in the <code>userAccountList</code> to the set of UserAccounts.
     *
     * @param userAccountList - the List of UserAccount objects to add
     */
    public void setUserAccounts(List userAccountList) {
        for (int i = 0; i < userAccountList.size(); i++) {
            UserAccount userAccount = (UserAccount) userAccountList.get(i);
            userAccounts.put(userAccount.getUsername(), userAccount);
        }
    }

}