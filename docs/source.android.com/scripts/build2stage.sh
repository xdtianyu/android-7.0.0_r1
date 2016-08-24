#!/bin/bash
#
# How to use:
# From directory {root}/docs/source.android.com/scripts run:
#    $ ./build2stage.sh <server number>
#
# For example, to build and stage on staging instance 13, run:
#    $ ./build2stage.sh 13
#

echo  'Please run this script from the docs/source.android.com/scripts directory ' \
  ' branch/docs/source.android.com/scripts'
echo ' '

# Read the configuration file to retrieve the App Engine staging - AE_STAGING - value
source /etc/profile.d/build2stage-conf.sh

# Go up three directories to build content
cd ../../..

# Delete old output
rm -rf out/target/common/docs/online-sac*

# Initialize the environment
source build/envsetup.sh

# Set up the Java environment
set_stuff_for_environment

# Note: if that stops working, try the lunch command with any build target,
# For example:
# lunch aosp_arm-eng

# Make the docs
make online-sac-docs

# Go to the output directory to stage content
cd out/target/common/docs

# Edit the app.yaml file to upload to the specified server.
sed 's/staging[0-9]*$/staging'$1'/' online-sac/app.yaml >  .temp

# Copy in new app.yaml content
cp .temp online-sac/app.yaml
rm .temp

# Stage the data on the server.
$AE_STAGING update online-sac

echo 'Your staged content is available at staging instance '$1''
