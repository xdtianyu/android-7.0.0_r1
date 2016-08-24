#!/bin/bash

if [ $# -ne 2 ]; then
  echo "Usage: $0 <path_to_frameworks_support> <support_version>"
  echo "  <path_to_frameworks_support>: Path to frameworks/support"
  echo "  <support_version>: Support version. This must match the API filenames exactly"
  echo 
  echo "  Example: import_api.sh ../../../frameworks/support 22.0.0"
  exit 1
fi

# Make sure we are in prebuilts/sdk/support-api
if [ $(realpath $(dirname $0)) != $(realpath $(pwd)) ]; then
  echo "The script must be run from $(dirname $0)."
  exit 1
fi

# Remove any existing file (if they exist)
if [ -e "$2.txt" ]; then
  rm "$2.txt"
fi

# Now create the concatentated API file
touch "$2.txt"

# Find all of the various API files and append them to our
# API file

for API_FILE in `find $1 -name "$2.txt" -print | sort`; do
  cat $API_FILE >> "$2.txt"
done
