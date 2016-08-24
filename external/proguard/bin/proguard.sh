#!/bin/sh
#
# Start-up script for ProGuard -- free class file shrinker, optimizer,
# obfuscator, and preverifier for Java bytecode.
#
# Note: when passing file names containing spaces to this script,
#       you'll have to add escaped quotes around them, e.g.
#       "\"/My Directory/My File.txt\""

# Account for possibly missing/basic readlink.
# POSIX conformant (dash/ksh/zsh/bash).
PROGUARD=`readlink -f "$0" 2>/dev/null`
if test "$PROGUARD" = ''
then
  PROGUARD=`readlink "$0" 2>/dev/null`
  if test "$PROGUARD" = ''
  then
    PROGUARD="$0"
  fi
fi

PROGUARD_HOME=`dirname "$PROGUARD"`/..

# BEGIN android-changed Added -Xmx2G for Mac builds
java -Xmx2G -jar "$PROGUARD_HOME/lib/proguard.jar" "$@"
# END android-changed
