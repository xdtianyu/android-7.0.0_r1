#!/bin/bash -ex
# Download & build ninja on the local machine
# works on Linux, OSX, and Windows (Git Bash)
# leaves output in /tmp/prebuilts/ninja/$OS-x86/

PROJ=ninja
VER=master
BASE_VER=v1.6.0
MSVS=2013

source $(dirname "$0")/build-common.sh build-common.sh

# needed for cygwin
export PATH="$PATH":.

# ninja specific steps
cd $RD
git clone https://android.googlesource.com/platform/external/ninja.git src
cd src
git remote add upstream https://github.com/martine/ninja.git
git fetch upstream
git checkout $VER
INSTALL_VER=${INSTALL_VER/${VER}/${VER}-$(git rev-parse --short=12 HEAD)}
if [[ "$OS" == "windows" ]] ; then
	 PLATFORM="--platform=msvc"
fi
./configure.py --bootstrap $PLATFORM

# install
cp $RD/src/ninja $INSTALL

EXTRA_FILE="LICENSE MODULE_LICENSE_APL"
EXTRA_COMMIT_MSG=$(echo -e "\n\nChanges since ${BASE_VER}:" && git log --oneline --abbrev=12 ${BASE_VER}..HEAD)

commit_and_push
