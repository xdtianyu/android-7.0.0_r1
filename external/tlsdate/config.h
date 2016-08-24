/* config.h.  Generated from config.in by configure.  */
/* config.in.  Generated from configure.ac by autoheader.  */


#pragma once

/* _SYS_FEATURE_TESTS_H is Solaris, _FEATURES_H is GCC */
#if defined( _SYS_FEATURE_TESTS_H) || defined(_FEATURES_H)
#error "You should include config.h as your first include file"
#endif

                

/* DBus client group */
#define DBUS_CLIENT_GROUP "root"

/* Another magical number */
/* #undef EAI_SYSTEM */

/* Defined if we are to build for an Android system */
#define HAVE_ANDROID 1

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Enable CrOS support */
/* #undef HAVE_CROS */

/* dbus enabled */
/* #undef HAVE_DBUS */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the `fmemopen' function. */
#define HAVE_FMEMOPEN 1

/* Define to 1 if you have the `funopen' function. */
#define HAVE_FUNOPEN 1

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the <grp.h> header file. */
#define HAVE_GRP_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <linux/rtc.h> header file. */
#define HAVE_LINUX_RTC_H 1

/* Define to 1 if you have the <mach/clock.h> header file. */
/* #undef HAVE_MACH_CLOCK_H */

/* Define to 1 if you have the <mach/mach.h> header file. */
/* #undef HAVE_MACH_MACH_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <openssl/bio.h> header file. */
#define HAVE_OPENSSL_BIO_H 1

/* Define to 1 if you have the <openssl/err.h> header file. */
#define HAVE_OPENSSL_ERR_H 1

/* Define to 1 if you have the <openssl/evp.h> header file. */
#define HAVE_OPENSSL_EVP_H 1

/* Define to 1 if you have the <openssl/ssl.h> header file. */
#define HAVE_OPENSSL_SSL_H 1

/* Define to 1 if you have the `prctl' function. */
#define HAVE_PRCTL 1

/* Define to 1 if you have the `preadv' function. */
/* #undef HAVE_PREADV */

/* Define to 1 if you have the <pwd.h> header file. */
#define HAVE_PWD_H 1

/* Define to 1 if you have the `pwritev' function. */
/* #undef HAVE_PWRITEV */

/* Enable seccomp filter */
#define HAVE_SECCOMP_FILTER 1

/* Define to 1 if you have the `setresuid' function. */
#define HAVE_SETRESUID 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strchrnul' function. */
/* #undef HAVE_STRCHRNUL */

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strnlen' function. */
#define HAVE_STRNLEN 1

/* Define to 1 if the system has the type `struct rtc_time'. */
#define HAVE_STRUCT_RTC_TIME 1

/* Define to 1 if you have the <sys/mman.h> header file. */
#define HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 or 0, depending whether the compiler supports simple visibility
   declarations. */
#define HAVE_VISIBILITY 1

/* CPU of Build System */
/* #undef HOST_CPU */

/* OS of Build System */
/* #undef HOST_OS */

/* Vendor of Build System */
/* #undef HOST_VENDOR */

/* User-Agent value to send when running as an HTTPS client */
#define HTTPS_USER_AGENT "TLSDate/0.0.13"

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "tlsdate"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "jacob at appelbaum.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "tlsdate"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "tlsdate 0.0.13"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "tlsdate"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.0.13"

/* TODO Automate conditional definition of this symbol. */
/* Time in seconds since the Disco epoch at build time */
#ifndef RECENT_COMPILE_DATE
# define RECENT_COMPILE_DATE 1440540554L
#endif

/* Enable seccomp filter debugging */
/* #undef SECCOMP_FILTER_DEBUG */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* CPU of Target System */
/* #undef TARGET_CPU */

/* OS of Target System */
/* #undef TARGET_OS */

/* Whether we are building for some other *BSD */
/* #undef TARGET_OS_BSD */

/* Whether we build for Cygwin */
/* #undef TARGET_OS_CYGWIN */

/* Whether we are building for DragonFly BSD */
/* #undef TARGET_OS_DRAGONFLYBSD */

/* Whether we are building for FreeBSD */
/* #undef TARGET_OS_FREEBSD */

/* Whether we build for GNU/Hurd */
/* #undef TARGET_OS_GNUHURD */

/* Whether we are building for GNU/kFreeBSD */
/* #undef TARGET_OS_GNUKFREEBSD */

/* Whether we build for Haiku */
/* #undef TARGET_OS_HAIKU */

/* Whether we build for Linux */
#define TARGET_OS_LINUX 1

/* Whether we build for MinGW */
/* #undef TARGET_OS_MINGW */

/* Whether we are building for NetBSD */
/* #undef TARGET_OS_NETBSD */

/* Whether we are building for OpenBSD */
/* #undef TARGET_OS_OPENBSD */

/* Whether we build for OSX */
/* #undef TARGET_OS_OSX */

/* Whether we are building for Solaris */
/* #undef TARGET_OS_SOLARIS */

/* Whether we are building for Windows */
/* #undef TARGET_OS_WINDOWS */

/* Vendor of Target System */
/* #undef TARGET_VENDOR */

/* Unprivileged group */
#define UNPRIV_GROUP "tlsdate"

/* Unprivileged user */
#define UNPRIV_USER "tlsdate"

/* if PolarSSL is enabled */
/* #undef USE_POLARSSL */

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* Version number of package */
#define VERSION "0.0.13"

/* Version of Windows */
/* #undef WINVER */

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Magical number to make things work */
/* #undef _WIN32_WINNT */


#ifndef HAVE_SYS_SOCKET_H
# define SHUT_RD SD_RECEIVE
# define SHUT_WR SD_SEND
# define SHUT_RDWR SD_BOTH
#endif
          
