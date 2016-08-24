This file describes how to compile dbus using the cmake build system

Requirements
------------
- cmake version >= 2.4.4 see http://www.cmake.org
- installed libexpat see http://sourceforge.net/projects/expat/ 
    unsupported RelWithDebInfo builds could be fetched 
    from http://sourceforge.net/projects/kde-windows/files/expat/

Building
--------

Win32 MinGW-w64|32
1. install mingw-w64 from http://sourceforge.net/projects/mingw-w64/
2. install cmake and libexpat
3. get dbus sources
4. unpack dbus sources into a sub directory (referred as <dbus-src-root> later)
5. mkdir dbus-build
6. cd dbus-build
7. run 
    cmake -G "MinGW Makefiles" [<options, see below>] <dbus-src-root>/cmake
    mingw32-make
    mingw32-make install

Win32 Microsoft nmake
1. install MSVC 2010 Express Version from http://www.microsoft.com/visualstudio/en-us/products/2010-editions/visual-cpp-express
2. install cmake and libexpat
3. get dbus sources
4. unpack dbus sources into a sub directory (referred as <dbus-src-root> later)
5. mkdir dbus-build
6. cd dbus-build
7. run 
    cmake -G "NMake Makefiles" [<options, see below>] <dbus-src-root>/cmake
    nmake
    nmake install

Win32 Visual Studio 2010 Express IDE
1. install MSVC 2010 Express Version from http://www.microsoft.com/visualstudio/en-us/products/2010-editions/visual-cpp-express
2. install cmake and libexpat
3. get dbus sources
4. unpack dbus sources into a sub directory (referred as <dbus-src-root> later)
5. mkdir dbus-build
6. cd dbus-build
7. run
      cmake -G "Visual Studio 10" [<options, see below>] <dbus-src-root>/cmake
8a. open IDE with
      vcexpress dbus.sln
8b. for immediate build run
      vcexpress dbus.sln /build

Win32 Visual Studio 2010 Professional IDE
1. install MSVC 2010 Professional Version
2. install cmake and libexpat
3. get dbus sources
4. unpack dbus sources into a sub directory (referred as <dbus-src-root> later)
5. mkdir dbus-build
6. cd dbus-build
7. run 
      cmake -G "Visual Studio 10" [<options, see below>] <dbus-src-root>/cmake
8a. open IDE with
      devenv dbus.sln
8b. for immediate build run
      devenv dbus.sln /build

Linux
1. install cmake and libexpat
2. get dbus sources
3. unpack dbus sources into a sub directory (referred as <dbus-src-root> later)
4. mkdir dbus-build
5. cd dbus-build
6. run 
    cmake -G "<for available targets, see cmake --help for a list>" [<options, see below>] <dbus-src-root>/cmake
    make
    make install

For other compilers see cmake --help in the Generators section

Configuration flags
-------------------

When using the cmake build system the dbus-specific configuration flags that can be given 
to the cmake program are these (use -D<key>=<value> on command line). The listed values 
are the defaults.

// Choose the type of build, options are: None(CMAKE_CXX_FLAGS or
// CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.
CMAKE_BUILD_TYPE:STRING=Debug

// Include path for 3rdparty packages
CMAKE_INCLUDE_PATH:PATH=

// Library path for 3rdparty packages
CMAKE_LIBRARY_PATH:PATH=

// Install path prefix, prepended onto install directories.
CMAKE_INSTALL_PREFIX:PATH=C:/Program Files/dbus


// enable unit test code
DBUS_BUILD_TESTS:BOOL=ON

// The name of the dbus daemon executable
DBUS_DAEMON_NAME:STRING=dbus-daemon

// Disable assertion checking
DBUS_DISABLE_ASSERTS:BOOL=OFF

// Disable public API sanity checking
DBUS_DISABLE_CHECKS:BOOL=OFF

// enable -ansi -pedantic gcc flags
DBUS_ENABLE_ANSI:BOOL=OFF

// build DOXYGEN documentation (requires Doxygen)
DBUS_ENABLE_DOXYGEN_DOCS:BOOL=OFF

// enable bus daemon usage statistics
DBUS_ENABLE_STATS:BOOL=OFF

// support verbose debug mode
DBUS_ENABLE_VERBOSE_MODE:BOOL=ON

// build XML  documentation (requires xmlto or meinproc4)
DBUS_ENABLE_XML_DOCS:BOOL=ON

// Some atomic integer implementation present
DBUS_HAVE_ATOMIC_INT:BOOL=OFF

// install required system libraries
DBUS_INSTALL_SYSTEM_LIBS:BOOL=OFF

// session bus default address
DBUS_SESSION_BUS_DEFAULT_ADDRESS:STRING=nonce-tcp:

// system bus default address
DBUS_SYSTEM_BUS_DEFAULT_ADDRESS:STRING=nonce-tcp:

// Use atomic integer implementation for 486
DBUS_USE_ATOMIC_INT_486:BOOL=OFF

// Use expat (== ON) or libxml2 (==OFF)
DBUS_USE_EXPAT:BOOL=ON

win32 only:
// enable win32 debug port for message output
DBUS_USE_OUTPUT_DEBUG_STRING:BOOL=OFF

gcc only:
// compile with coverage profiling instrumentation
DBUS_GCOV_ENABLED:BOOL=OFF

linux only:
// build with dnotify support 
DBUS_BUS_ENABLE_DNOTIFY_ON_LINUX:BOOL=ON

solaris only:
// enable console owner file 
HAVE_CONSOLE_OWNER_FILE:BOOL=ON

// Directory to check for console ownership
DBUS_CONSOLE_OWNER_FILE:STRING=/dev/console

x11 only:
// Build with X11 auto launch support
DBUS_BUILD_X11:BOOL=ON


Note: The above mentioned options could be extracted after 
configuring from the output of running "<maketool> help-options" 
in the build directory. The related entries start with 
CMAKE_ or DBUS_. 
