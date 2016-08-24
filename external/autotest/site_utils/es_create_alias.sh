#!/bin/bash
#
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

USAGE="Usage: ./es_create_alias.sh \
[-s <server>] [-p <port>] [-i <index>] [-a <alias>]
Example usage: ./es_create_alias.sh -s 172.25.61.45 -p 9200 -i \
test_index -a test_index_alias"

HELP="${USAGE}\n\n\
Create a new alias so that we can refer to an index via an alternate name.\n\
This is useful so we can remap an index without any downtime. \n\

Options:\n\
  -s IP of server running elasticsearch\n\
  -p Port of server running elasticsearch\n\
  -a A new name that we can refer to the index as \n\
  -i elasticsearch index, i.e. atlantis4.mtv, cautotest, localhost, etc.\n"

SERVER=
PORT=
ALIAS=
INDEX=
while getopts ":s:p:a:i:" opt; do
  case $opt in
    s)
      SERVER=$OPTARG
      ;;
    p)
      PORT=$OPTARG
      ;;
    a)
      ALIAS=$OPTARG
      ;;
    i)
      INDEX=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      echo "${USAGE}" >&2
      exit 1
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      echo "${USAGE}" >&2
      exit 1
      ;;
  esac
done

echo "Creating alias ${ALIAS} for index ${INDEX} for
      ES server at: ${SERVER}:${PORT}..."


curl -XPOST ${SERVER}:${PORT}/_aliases -d '
{
    "actions": [
        { "add": {
            "alias": '"\"${ALIAS}\""',
            "index": '"\"${INDEX}\""'
        }}
    ]
}'

echo ''