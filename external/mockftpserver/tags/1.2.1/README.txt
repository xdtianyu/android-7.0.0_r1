MockFtpServer version ${project.version}
-------------------------------------------------------------------------------
${project.url}

The MockFtpServer project provides a mock/dummy FTP server implementation that can be very 
useful for testing of FTP client code. It can be configured to return custom data or reply 
codes, to simulate either success or failure scenarios. Expected command invocations can 
also be verified. 

MockFtpServer is written in Java, and is ideally suited to testing Java code. But because 
communication with the FTP server is across sockets and TCP/IP, it can be used to test FTP client 
code written in any language.

The MockFtpServer project may one day provide multiple mock/dummy FTP server implementations,
at different levels of abstraction. Currently, however, StubFtpServer is the only one provided, 
though others are being considered. StubFtpServer is a "stub" implementation of an FTP server. 
See the "StubFtpServer Getting Started Guide" for more information.

See the FTP Protocol Spec (http://www.ietf.org/rfc/rfc0959.txt) for information about 
FTP, commands, reply codes, etc..

DEPENDENCIES

MockFtpServer requires 
 - Java (JDK) version 1.3 or later
 - The Log4J jar, version 1.2.13 or later, accessible on the CLASSPATH
   (http://logging.apache.org/log4j/index.html).
