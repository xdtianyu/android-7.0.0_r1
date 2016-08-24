# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

die () {
  echo "$@"
  exit 1
}

needuid () {
  uid=$(id -ru)
  [ "$uid" != "$1" ] && die "uid $uid != $1"
}

needeuid () {
  euid=$(id -u)
  [ "$euid" != "$1" ] && die "euid $euid != $1"
}

needgid () {
  gid=$(id -rg)
  [ "$gid" != "$1" ] && die "gid $gid != $1"
}


needegid () {
  egid=$(id -g)
  [ "$egid" != "$1" ] && die "egid $egid != $1"
}

needreuid () {
  needuid "$1"
  needeuid "$1"
}

needregid () {
  needgid "$1"
  needegid "$1"
}
