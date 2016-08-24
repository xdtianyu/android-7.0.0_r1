#! /bin/bash
# Copyright (c) 2016, Intel Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cd "$ANDROID_BUILD_TOP"
. build/envsetup.sh
cd -

set -eo pipefail

# INSTRUCTIONS
#
# Prior to running this script you should git-rm the previous content of the
# directory pointed to by the "asio" symbolic link, extract the version you
# want to integrate and finally, make the "asio" symbolic link point to the
# extracted directory. Additionaly, the build environment must be ready; i.e.
# you must have sourced "build/envestup.sh" and lunched any target.
#
# Then, run this script by passing it a list of lunch targets. ASIO will be
# minified based on which files were used when building the Parameter Framework
# on those lunch targets. As results may very among targets, it is advised to
# pass lunch targets of various architectures (ideally, all lunch targets).

# USAGE
# See the usage() function below

usage () {
    echo "run from the Parameter Framework's git root"
    echo "\tsupport/android/asio/asio_shrinker.sh <lunch target> [more lunch targets]"
    exit 1
}

fail () {
    code=$?
    echo "Asio shrinker error: $1"
    exit $code
}

list_compiled_files () {
    directory=$1
    output=$2

    # list all files used during compilation. Ignore "grep" errors: some .P
    # files may well not contain any "asio" line.
    find "$directory/obj" -name "*.P" | \
        xargs grep --no-filename 'external/parameter-framework/asio' >> "$output" || true
}

if [ $# -eq 0 ]; then
    echo "Not enough arguments."
    usage
fi

# This script must be run from the Parameter Framework's git root.
cd asio

asio_includes=$(mktemp)
find include -type f -name '*.[ih]pp' > "$asio_includes"

# unifdef can't parse multi-line macros. Help him out by removing the wrapping
xargs sed -i -e :a -e '/\\$/N' -e 's@\\ *\n@ @' -e ta < "$asio_includes"

# apply macro definitions
# "-x 2" means: only exit with a code != 0 if an error happened
# "-m" means: work on multiple files
# -f" means: read define and undef directives from the following file
xargs unifdef -x 2 -m -f ../support/android/asio/asio_defines.txt < "$asio_includes"
rm "$asio_includes"


# Take a list of lunch targets as argument and run "mma" on all of them.
# Why? because it seems that different target architectures require different
# sets of files
pushd ..
tmpfile=$(mktemp)
for target in "$@"; do
    lunch $target || fail "Failed to lunch $target"

    # compile and list the source files that actually have been used
    mma -j$(nproc) || fail "Failed to build $target"
    list_compiled_files "$ANDROID_PRODUCT_OUT" $tmpfile
done
popd
list_compiled_files "$ANDROID_HOST_OUT" $tmpfile

# In .P files, a line may contain several entries and may be terminated by " \"
# or " :" or nothing. Let's make sure we have one entry per line.
sed -r -e 's@^ *([^:\\]*)( [:\\])?$@\1@' \
    -e 's@^external/parameter-framework/asio/@@g' \
    -e 's@ @\n@' $tmpfile | \
        sort -u | \
        xargs git add || fail "Failed to git-add some necessary ASIO headers"
rm $tmpfile

# Add copyright mentions and readmes
git add COPYING LICENSE_1_0.txt README
# Remove asio.hpp because we override it
git rm -f include/asio.hpp || true # it may already have been removed

cat > README.parameter-framework << __EOF__
This version of ASIO has been minified for lower disk footprint for Android
integraton by support/android/asio/asio_shrinker.sh.

Although unlikely (thanks to having been tested on all AOSP lunch targets), if
the Parameter Framework fails to compile on your lunch target because of a
missing ASIO header, you may download a vanilla ASIO version from
'http://sourceforge.net/projects/asio/files/asio/1.10.6%20%28Stable%29/' and
run that script anew.
__EOF__
git add README.parameter-framework
