#!/bin/bash
#
# Copyright 2011 Google Inc. All Rights Reserved.
# Author: asharif@google.com (Ahmad Sharif)

for f in *_whitelist
do
  sort -o $f $f
done
