#!/bin/bash
#
# Copyright 2013 The Android Open Source Project.
#
# Retrieves the current Mockito source code into the current directory, excluding portions related
# to mockito's internal build system and javadoc.

SOURCE="git://github.com/mockito/mockito.git"
INCLUDE="
    LICENSE
    cglib-and-asm
    src
    "

EXCLUDE="
    cglib-and-asm/lib
    cglib-and-asm/.project
    cglib-and-asm/.classpath
    cglib-and-asm/build.gradle
    cglib-and-asm/mockito-repackaged.iml
    "

working_dir="$(mktemp -d)"
trap "echo \"Removing temporary directory\"; rm -rf $working_dir" EXIT

echo "Fetching Mockito source into $working_dir"
git clone $SOURCE $working_dir/source

for include in ${INCLUDE}; do
  echo "Updating $include"
  rm -rf $include
  cp -R $working_dir/source/$include .
done;

for exclude in ${EXCLUDE}; do
  echo "Excluding $exclude"
  rm -r $exclude
done;

echo "Done"

