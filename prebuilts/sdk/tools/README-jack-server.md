This documentation describes usage of Jack server 1.3-a8.

# Jack server

The Jack server's goal is to handle a pool of Jack compiler instances in order to limit memory usage
and benefit from already warm instances.



## Setup for Mac OS

Jack server is automatically installed and started by android build but requires initial setup on
Mac OS:

  - Install MacPorts from [macports.org](http://www.macports.org/install.php)  
    Make sure that `/opt/local/bin appears` in your path before `/usr/bin`. If not, please
    add the following to your `~/.bash_profile` file (If you do not have a `.bash_profile`
    file in your home directory, create one):
```  
    export PATH=/opt/local/bin:$PATH`
```

  - Get curl package from MacPorts:
```
    $ POSIXLY_CORRECT=1 sudo port install curl +ssl
```

## Starting the server

You may need to start the Jack server manually.  
Use `jack-admin start-server`.



## Client info

The client is a bash script simply named `jack`.  
It can be configured in `$HOME/.jack-settings`



### `.jack-settings` file

This file contains script shell variables:  
`SERVER_HOST`: IP address of the server  
by default: `SERVER_HOST=127.0.0.1`.

`SERVER_PORT_SERVICE`: Server service TCP port number. Needs to match the service port
number defined in `$HOME/.jack-server/config.properties` on the server host
(See **Server info** below)  
by default: `SERVER_PORT_SERVICE=8076`.

`SERVER_PORT_ADMIN`: Server admin TCP port number. Needs to match the admin port number
defined in `$HOME/.jack-server/config.properties` on the server host (See **Server info** below)  
by default: `SERVER_PORT_ADMIN=8077`.

`SETTING_VERSION`: Internal, do not modify.



## Server info

The server is composed of 2 jars named `jack-server.jar` and `jack-launcher.jar`.

Check `Admin` section to know how to install and administrate the Jack server.

The server can also be configured in `$HOME/.jack-server/config.properties`.



### `config.properties` file

It contains Jack server configuration properties.  
Description with default values follows:

`jack.server.max-service=<number>`  
  Maximum number of simultaneous Jack tasks. Default is 4.

`jack.server.max-jars-size=<size-in-bytes>`  
  Maximum size for Jars, in bytes. `-1` means no limit. Default is 100 MiB.

`jack.server.time-out=<time-in-seconds>`  
  Time out delay before Jack gets to sleep. When Jack sleeps, its memory usage is reduced, but it is
  slower to wake up. `-1` means "do not sleep". Default is 2 weeks.

`jack.server.service.port=<port-number>`  
  Server service TCP port number. Default is 8076. Needs to match the service port defined in
  `$HOME/.jack-settings` on the client host (See Client section).

`jack.server.admin.port=<port-number>`  
   Server admin TCP port number. Default is 8077. Needs to match the service port defined in
   `$HOME/.jack-settings` on the client host (See Client section).

`jack.server.config.version=<version>`  
  Internal, do not modify.



### Server logs

Server logs can be found by running `jack-admin server-log`. Default location is
`$HOME/.jack-server/logs/`.



## Admin

The `jack-admin` bash script allows to install and administrate the Jack server.
Here are some commands:

`$ jack-admin help`  
Print help.

`$ jack-admin install-server jack-launcher.jar jack-server.jar`  
Install the Jack server.

`$ jack-admin uninstall-server`  
Uninstall the Jack server and all components.

`$ jack-admin list jack`  
List installed versions for Jack.

`$ jack-admin update jack jack.jar`  
Install or update a Jack jar.

`$ jack-admin start-server`  
Start the server.

`$ jack-admin stop-server`  
Stop the server after the last compilation is complete.

`$ jack-admin kill-server`  
Kill the server process immediately, interrupting abruptly ongoing compilations.

`$ jack-admin list-server`  
List Jack server processes.

`$ jack-admin server-stat`  
Print various info about the server and the host.

`$ jack-admin server-log`  
Print log pattern.

`$ jack-admin dump-report`  
Produce a report file that can be used to file a bug.


## Transitioning from server 1.1 (e.g. Marshmallow) to server 1.3 (e.g. N)

The old Jack server used a `$HOME/.jack` configuration file. It has now replaced by a
`$HOME/.jack-settings` and a `$HOME/.jack-server/config.properties`.
If those new files do not exist, run `jack-admin start-server` and they will be created.
If you had custom settings in your `$HOME/.jack`, here's how to adapt those.

`SERVER_PORT_SERVICE=XXXX`  
Replace with `SERVER_PORT_SERVICE=XXXX` in `$HOME/.jack-settings` and
`jack.server.service.port=XXXX` in `$HOME/.jack-server/config.properties`.

`SERVER_PORT_ADMIN=YYYY`  
Replace with `SERVER_PORT_ADMIN=YYYY` in `$HOME/.jack-settings` and
`jack.server.admin.port=YYYY` in `$HOME/.jack-server/config.properties`.

`SERVER_NB_COMPILE=N`  
Replace with `jack.server.max-service=N` in `$HOME/.jack-server/config.properties`.

`SERVER_TIMEOUT=ZZ`  
You can replace with `jack.server.time-out=ZZ`, but it is recommended to keep the default setting of
"7200" (2 hours).

Other settings in the `$HOME/.jack` configuration file do not need to be copied.
You should still keep your `$HOME/.jack` configuration file for the old Jack server because both
server versions can run simultaneously.


## Troubleshooting

Below you'll find some ways to solve some troubleshooting. If you don't find a solution, file a
bug and attach the file produced by `jack-admin dump-report`.


### If compilation fails on `No Jack server running`

See [Starting the server](#starting-the-server) above.


### If your computer becomes unresponsive during compilation or if you experience Jack compilations
failing on `Out of memory error.`:

You can improve the situation by reducing the number of jack simultaneous compilations by editing
your `$HOME/.jack-server/config.properties` and changing jack.server.max-service to a lower value.


### If you have trouble starting the server

This may mean that TCP ports are already in use on your computer. You can try modifying the ports
both in your client and server configurations. See the `Server info` and `Client info` sections.
If it doesn't solve the problem, please report and give us additional information by:  
  - Attaching your compilation log.  
  - Attaching the file produced by `jack-admin dump-report`


### If your commands fails on
`Failed to contact Jack server: Problem reading<your home>/.jack-server/client.pem`

This may mean that your server never managed to start, see
[If you have trouble starting the server](#if-you-have-trouble-starting-the-server) above.


### If your compilation gets stuck without any progress

Please report and give us additional information by attaching the file produced by
`jack-admin dump-report`.  
Then restart the server by issuing commands `jack-admin kill-server; jack-admin start-server`

