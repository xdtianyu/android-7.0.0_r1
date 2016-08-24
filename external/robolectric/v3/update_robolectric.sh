# Script to update robolectric with an up to date snapshot.
#
# 1) Build and install Robolectric to your local mvn repository.
#    ./install-robolectric.sh
#
# 2) Create an Android repo
#
# 3) Run this script from $ANDROID_ROOT_DIR
#    ./external/robolectric/v3/update_robolectric.sh

MVN_ROOT=$HOME/.m2/repository
ROBOLECTRIC_DIR=external/robolectric/v3

cp $MVN_ROOT/org/robolectric/robolectric/3.1-SNAPSHOT/robolectric-3.1-SNAPSHOT.jar $ROBOLECTRIC_DIR/.
cp $MVN_ROOT/org/robolectric/robolectric-annotations/3.1-SNAPSHOT/robolectric-annotations-3.1-SNAPSHOT.jar $ROBOLECTRIC_DIR/.
cp $MVN_ROOT/org/robolectric/robolectric-processor/3.1-SNAPSHOT/robolectric-processor-3.1-SNAPSHOT.jar $ROBOLECTRIC_DIR/.
cp $MVN_ROOT/org/robolectric/robolectric-resources/3.1-SNAPSHOT/robolectric-resources-3.1-SNAPSHOT.jar $ROBOLECTRIC_DIR/.
cp $MVN_ROOT/org/robolectric/robolectric-utils/3.1-SNAPSHOT/robolectric-utils-3.1-SNAPSHOT.jar $ROBOLECTRIC_DIR/.

cp $MVN_ROOT/org/robolectric/shadows-httpclient/3.1-SNAPSHOT/shadows-httpclient-3.1-SNAPSHOT.jar $ROBOLECTRIC_DIR/.
cp $MVN_ROOT/org/robolectric/shadows-support-v4/3.1-SNAPSHOT/shadows-support-v4-3.1-SNAPSHOT.jar $ROBOLECTRIC_DIR/.
cp $MVN_ROOT/org/robolectric/shadows-multidex/3.1-SNAPSHOT/shadows-multidex-3.1-SNAPSHOT.jar $ROBOLECTRIC_DIR/.

cp $MVN_ROOT/org/robolectric/shadows-play-services/3.1-SNAPSHOT/shadows-play-services-3.1-SNAPSHOT.jar $ROBOLECTRIC_DIR/.

cp $MVN_ROOT/org/robolectric/shadows-core/3.1-SNAPSHOT/shadows-core-3.1-SNAPSHOT-16.jar $ROBOLECTRIC_DIR/runtime/.
cp $MVN_ROOT/org/robolectric/shadows-core/3.1-SNAPSHOT/shadows-core-3.1-SNAPSHOT-17.jar $ROBOLECTRIC_DIR/runtime/.
cp $MVN_ROOT/org/robolectric/shadows-core/3.1-SNAPSHOT/shadows-core-3.1-SNAPSHOT-18.jar $ROBOLECTRIC_DIR/runtime/.
cp $MVN_ROOT/org/robolectric/shadows-core/3.1-SNAPSHOT/shadows-core-3.1-SNAPSHOT-19.jar $ROBOLECTRIC_DIR/runtime/.
cp $MVN_ROOT/org/robolectric/shadows-core/3.1-SNAPSHOT/shadows-core-3.1-SNAPSHOT-21.jar $ROBOLECTRIC_DIR/runtime/.
cp $MVN_ROOT/org/robolectric/shadows-core/3.1-SNAPSHOT/shadows-core-3.1-SNAPSHOT-22.jar $ROBOLECTRIC_DIR/runtime/.
